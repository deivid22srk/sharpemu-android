#include "sharpemu_memory.h"
#include <sys/mman.h>
#include <android/log.h>
#include <cstring>
#include <algorithm>

#define LOG_TAG "SharpEmu-Memory"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

namespace SharpEmu {

GuestMemoryManager::GuestMemoryManager() = default;

GuestMemoryManager::~GuestMemoryManager() {
    Shutdown();
}

bool GuestMemoryManager::Initialize() {
    std::lock_guard lock(mutex_);
    if (initialized_) return true;

    void* base = mmap(
        reinterpret_cast<void*>(GUEST_BASE_ADDRESS),
        0x100000000ULL, // 4GB initial reservation
        PROT_NONE,
        MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE,
        -1, 0);

    if (base == MAP_FAILED) {
        base = mmap(nullptr, 0x100000000ULL, PROT_NONE,
                    MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
        if (base == MAP_FAILED) {
            LOGE("Failed to reserve guest address space");
            return false;
        }
    }

    next_alloc_addr_ = reinterpret_cast<uint64_t>(base);
    regions_.push_back({
        next_alloc_addr_, 0x100000000ULL, base, false, false
    });

    initialized_ = true;
    LOGI("Guest memory initialized at 0x%lx", next_alloc_addr_);
    return true;
}

void GuestMemoryManager::Shutdown() {
    std::lock_guard lock(mutex_);
    for (auto& region : regions_) {
        if (region.host_ptr && region.host_ptr != MAP_FAILED) {
            munmap(region.host_ptr, region.size);
        }
    }
    regions_.clear();
    initialized_ = false;
}

uint64_t GuestMemoryManager::MapMemory(uint64_t guest_addr, size_t size, bool executable, bool writable) {
    std::lock_guard lock(mutex_);

    size = (size + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);

    int prot = PROT_READ;
    if (writable) prot |= PROT_WRITE;
    if (executable) prot |= PROT_EXEC;

    void* host_ptr = mmap(
        reinterpret_cast<void*>(guest_addr),
        size,
        prot,
        MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED,
        -1, 0);

    if (host_ptr == MAP_FAILED) {
        host_ptr = mmap(nullptr, size, prot,
                        MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (host_ptr == MAP_FAILED) {
            LOGE("Failed to map memory at 0x%lx size 0x%zx", guest_addr, size);
            return 0;
        }
    }

    regions_.push_back({
        reinterpret_cast<uint64_t>(host_ptr),
        size,
        host_ptr,
        executable,
        writable
    });

    return reinterpret_cast<uint64_t>(host_ptr);
}

bool GuestMemoryManager::UnmapMemory(uint64_t guest_addr, size_t size) {
    std::lock_guard lock(mutex_);
    auto it = std::find_if(regions_.begin(), regions_.end(),
        [guest_addr](const MappedRegion& r) { return r.guest_base == guest_addr; });

    if (it != regions_.end()) {
        munmap(it->host_ptr, it->size);
        regions_.erase(it);
        return true;
    }
    return false;
}

bool GuestMemoryManager::WriteMemory(uint64_t guest_addr, const void* data, size_t size) {
    void* host = GetHostPointer(guest_addr);
    if (!host) return false;
    memcpy(host, data, size);
    return true;
}

bool GuestMemoryManager::ReadMemory(uint64_t guest_addr, void* data, size_t size) {
    void* host = GetHostPointer(guest_addr);
    if (!host) return false;
    memcpy(data, host, size);
    return true;
}

uint64_t GuestMemoryManager::AllocateGuestMemory(size_t size, bool executable) {
    std::lock_guard lock(mutex_);
    size = (size + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);

    int prot = PROT_READ | PROT_WRITE;
    if (executable) prot |= PROT_EXEC;

    void* ptr = mmap(
        reinterpret_cast<void*>(next_alloc_addr_),
        size,
        prot,
        MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED,
        -1, 0);

    if (ptr == MAP_FAILED) {
        ptr = mmap(nullptr, size, prot, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (ptr == MAP_FAILED) return 0;
    }

    uint64_t addr = reinterpret_cast<uint64_t>(ptr);
    next_alloc_addr_ = addr + size;

    regions_.push_back({addr, size, ptr, executable, true});
    return addr;
}

void GuestMemoryManager::FreeGuestMemory(uint64_t guest_addr, size_t size) {
    UnmapMemory(guest_addr, size);
}

void* GuestMemoryManager::GetHostPointer(uint64_t guest_addr) const {
    std::lock_guard lock(mutex_);
    for (const auto& region : regions_) {
        if (guest_addr >= region.guest_base &&
            guest_addr < region.guest_base + region.size) {
            return reinterpret_cast<void*>(
                reinterpret_cast<uint64_t>(region.host_ptr) +
                (guest_addr - region.guest_base));
        }
    }
    return reinterpret_cast<void*>(guest_addr);
}

uint64_t GuestMemoryManager::GetGuestAddress(const void* host_ptr) const {
    return reinterpret_cast<uint64_t>(host_ptr);
}

bool GuestMemoryManager::IsMapped(uint64_t guest_addr) const {
    std::lock_guard lock(mutex_);
    for (const auto& region : regions_) {
        if (guest_addr >= region.guest_base &&
            guest_addr < region.guest_base + region.size) {
            return true;
        }
    }
    return false;
}

} // namespace SharpEmu
