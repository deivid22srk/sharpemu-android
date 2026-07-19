#include "sharpemu_syscall_handler.h"
#include <android/log.h>

#define LOG_TAG "SharpEmu-Syscall"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

namespace SharpEmu {

OrbisSyscallHandler::OrbisSyscallHandler() {
    OSABI = FEXCore::HLE::SyscallOSABI::OS_GENERIC;
}

OrbisSyscallHandler::~OrbisSyscallHandler() = default;

uint64_t OrbisSyscallHandler::HandleSyscall(FEXCore::Core::CpuStateFrame* Frame,
                                             FEXCore::HLE::SyscallArguments* Args) {
    int syscall_num = static_cast<int>(Args->Argument[0]);
    uint64_t args[6] = {
        Args->Argument[1], Args->Argument[2], Args->Argument[3],
        Args->Argument[4], Args->Argument[5], Args->Argument[6]
    };

    if (hle_dispatch_) {
        return hle_dispatch_(syscall_num, args);
    }

    switch (syscall_num) {
        case 0: // sys_exit
            LOGI("Guest called sys_exit(%lu)", args[0]);
            return 0;

        case 1: // sys_write (stdout/stderr)
            return args[2]; // return count as success

        case 2: // sys_read
            return 0;

        case 3: // sys_close
            return 0;

        case 5: // sys_open
            return static_cast<uint64_t>(-1); // ENOENT

        case 20: // sys_getpid
            return 1;

        case 73: // sys_munmap
            return 0;

        case 90: // sys_mmap (FreeBSD)
            return 0; // handled by memory manager

        case 197: // sys_mmap (Orbis)
            return 0;

        case 3: // sys_thr_exit (Orbis)
            return 0;

        case 431: // sys_thr_new (Orbis)
            return 0;

        case 432: // sys_thr_self (Orbis)
            return 1;

        case 466: // sys_rtprio_thread
            return 0;

        case 557: // sys_namedobj_create (Orbis)
            return 1;

        case 585: // sys_dynlib_load_prx (Orbis)
            LOGI("Guest dynlib_load_prx: %s", reinterpret_cast<const char*>(args[0]));
            return 0;

        case 591: // sys_dynlib_get_proc_param (Orbis)
            return 0;

        case 599: // sys_get_paging_stats_of_all_threads
            return 0;

        case 601: // sys_get_proc_type_info
            return 0;

        case 602: // sys_is_development_mode
            return 1; // devkit mode

        case 608: // sys_get_self_auth_info
            return 0;

        case 609: // sys_dynlib_get_info_ex
            return 0;

        case 610: // sys_budget_get_ptype
            return 0;

        case 611: // sys_get_paging_stats_of_all_objects
            return 0;

        case 612: // sys_get_proc_type_info
            return 0;

        case 613: // sys_is_in_sandbox
            return 0;

        case 617: // sys_get_phys_page_size
            return 0;

        case 621: // sys_get_vm_map_timestamp
            return 0;

        case 622: // sys_opmc_set_hw
            return 0;

        case 623: // sys_opmc_get_hw
            return 0;

        case 625: // sys_opmc_set_ctl
            return 0;

        case 626: // sys_opmc_set_ctr
            return 0;

        case 627: // sys_opmc_get_ctr
            return 0;

        case 628: // sys_budget_getid
            return 0;

        case 643: // sys_set_uevt
            return 0;

        case 649: // sys_soc_memory
            return 0;

        case 653: // sys_blockpool_open
            return 1;

        case 654: // sys_blockpool_map
            return 0;

        case 655: // sys_blockpool_unmap
            return 0;

        case 657: // sys_blockpool_batch
            return 0;

        case 658: // sys_fdatasync
            return 0;

        case 660: // sys_blockpool_move
            return 0;

        case 661: // sys_virtual_query
            return 0;

        case 669: // sys_mtypeprotect
            return 0;

        case 672: // sys_regmgr_call
            return 0;

        case 673: // sys_jitshm_create
            return 3; // fd

        case 674: // sys_jitshm_alias
            return 4; // fd

        case 675: // sys_dl_get_list
            return 0;

        case 676: // sys_dl_get_info
            return 0;

        case 678: // sys_evf_create
            return 1;

        case 679: // sys_evf_delete
            return 0;

        case 680: // sys_evf_open
            return 1;

        case 681: // sys_evf_close
            return 0;

        case 682: // sys_evf_wait
            return 0;

        case 683: // sys_evf_trywait
            return 0;

        case 684: // sys_evf_set
            return 0;

        case 685: // sys_evf_clear
            return 0;

        case 686: // sys_evf_cancel
            return 0;

        case 687: // sys_query_memory_protection
            return 0;

        case 688: // sys_batch_map
            return 0;

        case 689: // sys_osem_create
            return 1;

        case 690: // sys_osem_delete
            return 0;

        case 691: // sys_osem_open
            return 1;

        case 692: // sys_osem_close
            return 0;

        case 693: // sys_osem_wait
            return 0;

        case 694: // sys_osem_trywait
            return 0;

        case 695: // sys_osem_post
            return 0;

        case 696: // sys_osem_cancel
            return 0;

        case 697: // sys_namedobj_create
            return 1;

        case 698: // sys_namedobj_delete
            return 0;

        case 699: // sys_set_vm_container
            return 0;

        case 700: // sys_debug_init
            return 0;

        default:
            LOGW("Unhandled Orbis syscall %d (args: %lu %lu %lu)",
                 syscall_num, args[0], args[1], args[2]);
            return 0;
    }
}

FEXCore::HLE::ExecutableRangeInfo OrbisSyscallHandler::QueryGuestExecutableRange(
    FEXCore::Core::InternalThreadState* Thread, uint64_t Address) {
    std::lock_guard lock(regions_mutex_);
    for (const auto& region : exec_regions_) {
        if (Address >= region.base && Address < region.base + region.size) {
            return {region.base, region.size, region.writable};
        }
    }
    return {Address & ~0xFFFULL, 0x1000, false};
}

std::optional<FEXCore::HLE::ExecutableFileSectionInfo>
OrbisSyscallHandler::LookupExecutableFileSection(
    FEXCore::Core::InternalThreadState* Thread, uint64_t GuestAddr) {
    return std::nullopt;
}

void OrbisSyscallHandler::MarkGuestExecutableRange(
    FEXCore::Core::InternalThreadState* Thread, uint64_t Start, uint64_t Length) {
    RegisterExecutableRegion(Start, Length, false);
}

void OrbisSyscallHandler::SetImportCallback(ImportCallback callback) {
    import_callback_ = std::move(callback);
}

void OrbisSyscallHandler::RegisterExecutableRegion(uint64_t base, uint64_t size, bool writable) {
    std::lock_guard lock(regions_mutex_);
    exec_regions_.push_back({base, size, writable});
}

void OrbisSyscallHandler::SetHleDispatch(std::function<uint64_t(int, uint64_t*)> dispatch) {
    hle_dispatch_ = std::move(dispatch);
}

} // namespace SharpEmu
