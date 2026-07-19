#pragma once

#include <cstdint>
#include <cstddef>
#include <vector>
#include <mutex>

namespace SharpEmu {

class GuestMemoryManager {
public:
    static constexpr uint64_t PAGE_SIZE = 0x1000;
    static constexpr uint64_t GUEST_BASE_ADDRESS = 0x0000004000000000ULL;
    static constexpr uint64_t GUEST_MAX_ADDRESS = 0x00007FFFFFFFFFFFULL;

    GuestMemoryManager();
    ~GuestMemoryManager();

    bool Initialize();
    void Shutdown();

    uint64_t MapMemory(uint64_t guest_addr, size_t size, bool executable, bool writable);
    bool UnmapMemory(uint64_t guest_addr, size_t size);
    bool WriteMemory(uint64_t guest_addr, const void* data, size_t size);
    bool ReadMemory(uint64_t guest_addr, void* data, size_t size);
    uint64_t AllocateGuestMemory(size_t size, bool executable);
    void FreeGuestMemory(uint64_t guest_addr, size_t size);

    void* GetHostPointer(uint64_t guest_addr) const;
    uint64_t GetGuestAddress(const void* host_ptr) const;

    bool IsMapped(uint64_t guest_addr) const;

private:
    struct MappedRegion {
        uint64_t guest_base;
        uint64_t size;
        void* host_ptr;
        bool executable;
        bool writable;
    };

    mutable std::mutex mutex_;
    std::vector<MappedRegion> regions_;
    uint64_t next_alloc_addr_ = GUEST_BASE_ADDRESS;
    bool initialized_ = false;
};

} // namespace SharpEmu
