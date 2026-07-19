#include <jni.h>
#include <android/log.h>
#include <FEXCore/Core/Context.h>
#include <FEXCore/Core/CoreState.h>
#include <FEXCore/Core/HostFeatures.h>
#include <FEXCore/Config/Config.h>
#include <FEXCore/Utils/Allocator.h>
#include "sharpemu_syscall_handler.h"
#include "sharpemu_memory.h"

#include <memory>
#include <thread>
#include <atomic>

#define LOG_TAG "SharpEmu-FEX"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

namespace {

struct EmulatorState {
    std::unique_ptr<FEXCore::Context::Context> context;
    std::unique_ptr<SharpEmu::OrbisSyscallHandler> syscall_handler;
    std::unique_ptr<SharpEmu::GuestMemoryManager> memory;
    FEXCore::Core::InternalThreadState* guest_thread = nullptr;
    std::thread execution_thread;
    std::atomic<bool> running{false};
    std::atomic<bool> stop_requested{false};
    uint64_t entry_point = 0;
};

EmulatorState* g_state = nullptr;

FEXCore::HostFeatures DetectHostFeatures() {
    FEXCore::HostFeatures features{};
    features.DCacheLineSize = 64;
    features.ICacheLineSize = 64;
    features.SupportsCacheMaintenanceOps = true;
    features.SupportsAES = true;
    features.SupportsCRC = true;
    features.SupportsAtomics = true;
    features.SupportsRCPC = true;
    features.SupportsRAND = true;
    features.SupportsAVX = false;
    features.SupportsSVE128 = false;
    features.SupportsSVE256 = false;
    features.SupportsSHA = false;
    features.SupportsPMULL_128Bit = true;
    features.SupportsFlagM = true;
    features.SupportsFlagM2 = true;
    features.SupportsAFP = true;
    features.SupportsFloatExceptions = false;
    features.HostType = FEXCore::HostFeatures::HostTypeEnum::Linux;
    return features;
}

} // anonymous namespace

extern "C" {

JNIEXPORT jlong JNICALL
Java_com_sharpemu_android_emulation_FexEmulator_nativeCreate(JNIEnv* env, jobject thiz) {
    LOGI("Creating SharpEmu FEX emulator instance");

    auto state = new EmulatorState();
    state->memory = std::make_unique<SharpEmu::GuestMemoryManager>();
    state->syscall_handler = std::make_unique<SharpEmu::OrbisSyscallHandler>();

    if (!state->memory->Initialize()) {
        LOGE("Failed to initialize guest memory");
        delete state;
        return 0;
    }

    auto features = DetectHostFeatures();
    state->context = FEXCore::Context::Context::CreateNewContext(features);

    if (!state->context) {
        LOGE("Failed to create FEX context");
        delete state;
        return 0;
    }

    g_state = state;
    LOGI("FEX emulator instance created successfully");
    return reinterpret_cast<jlong>(state);
}

JNIEXPORT void JNICALL
Java_com_sharpemu_android_emulation_FexEmulator_nativeDestroy(JNIEnv* env, jobject thiz, jlong handle) {
    auto state = reinterpret_cast<EmulatorState*>(handle);
    if (!state) return;

    LOGI("Destroying FEX emulator instance");
    state->stop_requested = true;
    state->running = false;

    if (state->execution_thread.joinable()) {
        state->execution_thread.join();
    }

    state->context.reset();
    state->syscall_handler.reset();
    state->memory->Shutdown();
    state->memory.reset();

    if (g_state == state) g_state = nullptr;
    delete state;
}

JNIEXPORT jboolean JNICALL
Java_com_sharpemu_android_emulation_FexEmulator_nativeLoadElf(JNIEnv* env, jobject thiz,
                                                               jlong handle, jstring path) {
    auto state = reinterpret_cast<EmulatorState*>(handle);
    if (!state) return JNI_FALSE;

    const char* elf_path = env->GetStringUTFChars(path, nullptr);
    LOGI("Loading ELF: %s", elf_path);

    FILE* f = fopen(elf_path, "rb");
    if (!f) {
        LOGE("Cannot open ELF file: %s", elf_path);
        env->ReleaseStringUTFChars(path, elf_path);
        return JNI_FALSE;
    }

    fseek(f, 0, SEEK_END);
    long file_size = ftell(f);
    fseek(f, 0, SEEK_SET);

    std::vector<uint8_t> elf_data(file_size);
    fread(elf_data.data(), 1, file_size, f);
    fclose(f);
    env->ReleaseStringUTFChars(path, elf_path);

    if (file_size < 64) {
        LOGE("ELF file too small");
        return JNI_FALSE;
    }

    if (elf_data[0] != 0x7F || elf_data[1] != 'E' || elf_data[2] != 'L' || elf_data[3] != 'F') {
        LOGE("Invalid ELF magic");
        return JNI_FALSE;
    }

    bool is_64bit = (elf_data[4] == 2);
    if (!is_64bit) {
        LOGE("Only 64-bit ELF supported");
        return JNI_FALSE;
    }

    uint64_t e_entry = *reinterpret_cast<uint64_t*>(&elf_data[24]);
    uint16_t e_phnum = *reinterpret_cast<uint16_t*>(&elf_data[56]);
    uint64_t e_phoff = *reinterpret_cast<uint64_t*>(&elf_data[32]);
    uint16_t e_phentsize = *reinterpret_cast<uint16_t*>(&elf_data[54]);

    LOGI("ELF entry: 0x%lx, phnum: %d", e_entry, e_phnum);

    for (int i = 0; i < e_phnum; i++) {
        size_t ph_offset = e_phoff + i * e_phentsize;
        if (ph_offset + 56 > static_cast<size_t>(file_size)) break;

        uint32_t p_type = *reinterpret_cast<uint32_t*>(&elf_data[ph_offset]);
        if (p_type != 1) continue; // PT_LOAD only

        uint32_t p_flags = *reinterpret_cast<uint32_t*>(&elf_data[ph_offset + 4]);
        uint64_t p_offset = *reinterpret_cast<uint64_t*>(&elf_data[ph_offset + 8]);
        uint64_t p_vaddr = *reinterpret_cast<uint64_t*>(&elf_data[ph_offset + 16]);
        uint64_t p_filesz = *reinterpret_cast<uint64_t*>(&elf_data[ph_offset + 32]);
        uint64_t p_memsz = *reinterpret_cast<uint64_t*>(&elf_data[ph_offset + 40]);

        bool exec = (p_flags & 1) != 0;
        bool write = (p_flags & 2) != 0;

        uint64_t mapped = state->memory->MapMemory(p_vaddr, p_memsz, exec, true);
        if (mapped == 0) {
            LOGE("Failed to map segment at 0x%lx", p_vaddr);
            return JNI_FALSE;
        }

        if (p_filesz > 0 && p_offset + p_filesz <= static_cast<size_t>(file_size)) {
            void* dest = state->memory->GetHostPointer(p_vaddr);
            if (dest) {
                memcpy(dest, &elf_data[p_offset], p_filesz);
            }
        }

        if (exec) {
            state->syscall_handler->RegisterExecutableRegion(p_vaddr, p_memsz, write);
        }

        LOGI("Mapped segment: vaddr=0x%lx memsz=0x%lx exec=%d", p_vaddr, p_memsz, exec);
    }

    state->entry_point = e_entry;
    LOGI("ELF loaded successfully, entry=0x%lx", e_entry);
    return JNI_TRUE;
}

JNIEXPORT jboolean JNICALL
Java_com_sharpemu_android_emulation_FexEmulator_nativeStart(JNIEnv* env, jobject thiz, jlong handle) {
    auto state = reinterpret_cast<EmulatorState*>(handle);
    if (!state || state->running) return JNI_FALSE;

    LOGI("Starting guest execution at 0x%lx", state->entry_point);

    state->stop_requested = false;
    state->running = true;

    state->execution_thread = std::thread([state]() {
        LOGI("Guest execution thread started");

        if (!state->context->InitCore()) {
            LOGE("Failed to init FEX core");
            state->running = false;
            return;
        }

        auto thread_state = state->context->CreateThread(
            state->entry_point, 0, nullptr, 0, nullptr);

        if (!thread_state) {
            LOGE("Failed to create guest thread");
            state->running = false;
            return;
        }

        state->guest_thread = thread_state;
        state->context->ExecuteThread(thread_state);

        LOGI("Guest execution finished");
        state->running = false;
    });

    return JNI_TRUE;
}

JNIEXPORT void JNICALL
Java_com_sharpemu_android_emulation_FexEmulator_nativeStop(JNIEnv* env, jobject thiz, jlong handle) {
    auto state = reinterpret_cast<EmulatorState*>(handle);
    if (!state) return;

    LOGI("Stopping guest execution");
    state->stop_requested = true;

    if (state->guest_thread) {
        state->context->StopThread(state->guest_thread);
    }

    if (state->execution_thread.joinable()) {
        state->execution_thread.join();
    }

    state->running = false;
}

JNIEXPORT jboolean JNICALL
Java_com_sharpemu_android_emulation_FexEmulator_nativeIsRunning(JNIEnv* env, jobject thiz, jlong handle) {
    auto state = reinterpret_cast<EmulatorState*>(handle);
    return state ? static_cast<jboolean>(state->running.load()) : JNI_FALSE;
}

JNIEXPORT jlong JNICALL
Java_com_sharpemu_android_emulation_FexEmulator_nativeMapMemory(JNIEnv* env, jobject thiz,
                                                                 jlong handle, jlong addr,
                                                                 jlong size, jboolean exec,
                                                                 jboolean write) {
    auto state = reinterpret_cast<EmulatorState*>(handle);
    if (!state) return 0;
    return static_cast<jlong>(state->memory->MapMemory(
        static_cast<uint64_t>(addr), static_cast<size_t>(size), exec, write));
}

JNIEXPORT jboolean JNICALL
Java_com_sharpemu_android_emulation_FexEmulator_nativeWriteMemory(JNIEnv* env, jobject thiz,
                                                                    jlong handle, jlong addr,
                                                                    jbyteArray data) {
    auto state = reinterpret_cast<EmulatorState*>(handle);
    if (!state) return JNI_FALSE;

    jsize len = env->GetArrayLength(data);
    jbyte* buf = env->GetByteArrayElements(data, nullptr);
    bool ok = state->memory->WriteMemory(static_cast<uint64_t>(addr), buf, len);
    env->ReleaseByteArrayElements(data, buf, JNI_ABORT);
    return ok ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT void JNICALL
Java_com_sharpemu_android_emulation_FexEmulator_nativeSetHleDispatch(JNIEnv* env, jobject thiz,
                                                                      jlong handle) {
    auto state = reinterpret_cast<EmulatorState*>(handle);
    if (!state) return;

    JavaVM* jvm;
    env->GetJavaVM(&jvm);

    jobject global_ref = env->NewGlobalRef(thiz);

    state->syscall_handler->SetHleDispatch(
        [jvm, global_ref](int syscall_num, uint64_t* args) -> uint64_t {
            JNIEnv* jni_env;
            bool attached = false;
            if (jvm->GetEnv(reinterpret_cast<void**>(&jni_env), JNI_VERSION_1_6) != JNI_OK) {
                jvm->AttachCurrentThread(&jni_env, nullptr);
                attached = true;
            }

            jclass cls = jni_env->GetObjectClass(global_ref);
            jmethodID method = jni_env->GetMethodID(cls, "handleSyscall", "(I[J)J");

            jlongArray jargs = jni_env->NewLongArray(6);
            jlong arg_buf[6];
            for (int i = 0; i < 6; i++) arg_buf[i] = static_cast<jlong>(args[i]);
            jni_env->SetLongArrayRegion(jargs, 0, 6, arg_buf);

            uint64_t result = static_cast<uint64_t>(
                jni_env->CallLongMethod(global_ref, method, syscall_num, jargs));

            jni_env->DeleteLocalRef(jargs);
            jni_env->DeleteLocalRef(cls);

            if (attached) {
                jvm->DetachCurrentThread();
            }

            return result;
        });
}

} // extern "C"
