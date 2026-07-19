#pragma once

#include <FEXCore/HLE/SyscallHandler.h>
#include <FEXCore/Core/Context.h>
#include <cstdint>
#include <functional>
#include <unordered_map>
#include <string>
#include <mutex>

namespace SharpEmu {

using ImportCallback = std::function<uint64_t(uint64_t nid_hash, uint64_t* args, int num_args)>;

class OrbisSyscallHandler final : public FEXCore::HLE::SyscallHandler {
public:
    OrbisSyscallHandler();
    ~OrbisSyscallHandler() override;

    uint64_t HandleSyscall(FEXCore::Core::CpuStateFrame* Frame,
                           FEXCore::HLE::SyscallArguments* Args) override;

    FEXCore::HLE::ExecutableRangeInfo QueryGuestExecutableRange(
        FEXCore::Core::InternalThreadState* Thread, uint64_t Address) override;

    std::optional<FEXCore::HLE::ExecutableFileSectionInfo> LookupExecutableFileSection(
        FEXCore::Core::InternalThreadState* Thread, uint64_t GuestAddr) override;

    void MarkGuestExecutableRange(FEXCore::Core::InternalThreadState* Thread,
                                  uint64_t Start, uint64_t Length) override;

    void SetImportCallback(ImportCallback callback);
    void RegisterExecutableRegion(uint64_t base, uint64_t size, bool writable);
    void SetHleDispatch(std::function<uint64_t(int syscall_num, uint64_t* args)> dispatch);

private:
    ImportCallback import_callback_;
    std::function<uint64_t(int, uint64_t*)> hle_dispatch_;
    std::mutex regions_mutex_;

    struct ExecRegion {
        uint64_t base;
        uint64_t size;
        bool writable;
    };
    std::vector<ExecRegion> exec_regions_;
};

} // namespace SharpEmu
