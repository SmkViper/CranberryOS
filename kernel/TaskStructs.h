#ifndef KERNEL_TASK_STRUCTS_H
#define KERNEL_TASK_STRUCTS_H

#include <cstdint>

namespace Scheduler
{
    struct CPUContext
    {
        // ARM calling conventions allow registers x0-x18 to be overwritten by a called function,
        // so we don't need to save those off
        //
        // #TODO: May need to save off additional registers (i.e. SIMD/FP registers?)
        uint64_t x19 = 0;
        uint64_t x20 = 0;
        uint64_t x21 = 0;
        uint64_t x22 = 0;
        uint64_t x23 = 0;
        uint64_t x24 = 0;
        uint64_t x25 = 0;
        uint64_t x26 = 0;
        uint64_t x27 = 0;
        uint64_t x28 = 0;
        uint64_t fp = 0; // x29
        uint64_t sp = 0;
        uint64_t pc = 0; // x30
    };

    enum class TaskState : int64_t
    {
        Running,
        Zombie
    };

    constexpr std::size_t MaxProcessPagesCS = 16;

    // #TODO: We really should have the idea of "physical address" vs "kernel virtual address" vs "user virtual
    // address" as types to avoid mixing them up on accident

    struct UserPage
    {
        void* PhysicalAddress = nullptr;
        uintptr_t VirtualAddress = 0u;
    };

    struct MemoryManagerState
    {
        void* pPageGlobalDirectory = nullptr; // physical address
        uint32_t UserPagesCount = 0;
        UserPage UserPages[MaxProcessPagesCS] = {};
        uint32_t KernelPagesCount = 0;
        void* KernelPages[MaxProcessPagesCS] = {}; // physical addresses
    };

    struct TaskStruct
    {
        CPUContext Context;
        TaskState State = TaskState::Running;
        int64_t Counter = 0; // decrements each timer tick. When reaches 0, another task will be scheduled
        int64_t Priority = 1; // copied to Counter when a task is scheduled, so higher priority will run for longer
        int64_t PreemptCount = 0; // If non-zero, task will not be preempted
        uint64_t Flags = 0;
        MemoryManagerState MemoryState;
    };
} // Scheduler namespace

#endif // KERNEL_TASK_STRUCTS_H