#include "Scheduler.h"

#include <bit>
#include <cstddef>
#include <cstdint>
#include <cstring>
// Technically needed for placement new, but for some reason clang-tidy doesn't pick up on that
#include <new> // NOLINT(misc-include-cleaner)
#include "AArch64/SchedulerDefines.h"
#include "IRQ.h"
#include "MemoryManager.h"
#include "PointerTypes.h"
#include "TaskStructs.h"
#include "Timer.h"

// How the scheduler currently works:
//
// CopyProcess creates a new memory page and puts the task struct at the bottom of the page, with the stack pointer
// pointing at a certain distance above it.
//
// 0xXXXXXXXX +--------------------+ ^
//            | TaskStruct         | |
//            +--------------------+ | One page
//            |                    | |
//            | Stack (grows up)   | |
//            +--------------------+ |
//            | ProcessState       | |
// 0xXXXX1000 +--------------------+ v
//
// ScheduleImpl is called, either voluntarily or via timer
// cpu_switch_to saves all callee-saved registers in the current task to the TaskStruct context member
// cpu_switch_to "restores" all callee-saved registers for the new task, setting sp to 0xXXXX1000, the link register
// to ret_from_create, x19 to the task's process function, and x20 to the process function parameter
// cpu_switch_to returns, loading ret_from_create's address from the link register
// ret_from_create reads from x19 and x20, and calls to the function in x19, passing it x20
//
// Eventually a timer interrupt happens, saving all registers + elr_el1 and spsr_el1 to the bottom of the current
// task's stack
//
// 0xXXXXXXXX +----------------------+
//            | TaskStruct           |
//            +----------------------+
//            |                      |
//            +----------------------+
//            | Task saved registers |
//            +----------------------+
//            | Stack (grows up)     |
//            +----------------------+
//            | ProcessState         |
// 0xXXXX1000 +----------------------+
//
// The current task is now handling an interrupt, and grows a little bit more on the stack to pick the task to resume
//
// 0xXXXXXXXX +----------------------+
//            | TaskStruct           |
//            +----------------------+
//            |                      |
//            +----------------------+
//            | Stack (interrupt)    |
//            +----------------------+
//            | Task saved registers |
//            +----------------------+
//            | Stack (grows up)     |
//            +----------------------+
//            | ProcessState         |
// 0xXXXX1000 +----------------------+
//
// The interrupt picks a second new task to run, repeating the process performed for the first task to set up the
// second new task. This task begins executing and growing its own stack. Note that execution is still in the timer
// interrupt handler, but interrupts have been re-enabled at this point, so another timer can come in again.
//
// 0xXXXXXXXX +----------------------+
//            | TaskStruct           |
//            +----------------------+
//            |                      |
//            +----------------------+
//            | Stack (interrupt)    |
//            +----------------------+
//            | Task saved registers |
//            +----------------------+
//            | Stack (grows up)     |
//            +----------------------+
//            | ProcessState         |
// 0xXXXX1000 +----------------------+
//            |         ...          |
// 0xYYYYYYYY +----------------------+
//            | TaskStruct           |
//            +----------------------+
//            |                      |
//            | Stack (grows up)     |
//            +----------------------+
//            | ProcessState         |
// 0xYYYY1000 +----------------------+
//
// Another timer interrupt happens, and the process repeats to save off all the registers, elr_el1 and spsr_el1 at the
// bottom of the second task's stack, and the interrupt stack for that task starts to grow
//
// 0xXXXXXXXX +----------------------+
//            | TaskStruct           |
//            +----------------------+
//            |                      |
//            +----------------------+
//            | Stack (interrupt)    |
//            +----------------------+
//            | Task saved registers |
//            +----------------------+
//            | Stack (grows up)     |
//            +----------------------+
//            | ProcessState         |
// 0xXXXX1000 +----------------------+
//            |         ...          |
// 0xYYYYYYYY +----------------------+
//            | TaskStruct           |
//            +----------------------+
//            |                      |
//            +----------------------+
//            | Stack (interrupt)    |
//            +----------------------+
//            | Task saved registers |
//            +----------------------+
//            | Stack (grows up)     |
//            +----------------------+
//            | ProcessState         |
// 0xYYYY1000 +----------------------+
//
// ScheduleImpl is now called, and notes that both tasks have their counter at 0. It then sets the counters to their
// priority and picks the first task to run again. (Though it could pick either if their priorities were the same)
// cpu_switch_to is called and it restores all the callee-saved registers from the first task context. The link
// register now points at the end of the SwitchTo function, since that's what it was the last time this task was
// running. The stack pointer also is set to point at the bottom of the first task's interrupt stack.
// The TimerTick function now resumes execution, disabling interrupts again and returns to the interrupt handler,
// collapsing the interrupt stack to 0.
//
// 0xXXXXXXXX +----------------------+
//            | TaskStruct           |
//            +----------------------+
//            |                      |
//            +----------------------+
//            | Task saved registers |
//            +----------------------+
//            | Stack (grows up)     |
//            +----------------------+
//            | ProcessState         |
// 0xXXXX1000 +----------------------+
//            |         ...          |
// 0xYYYYYYYY +----------------------+
//            | TaskStruct           |
//            +----------------------+
//            |                      |
//            +----------------------+
//            | Stack (interrupt)    |
//            +----------------------+
//            | Task saved registers |
//            +----------------------+
//            | Stack (grows up)     |
//            +----------------------+
//            | ProcessState         |
// 0xYYYY1000 +----------------------+
//
// The interrupt cleans up, restoring all the regsters that were saved from the stack, including the elr_el1 and
// spsr_el1 registers. elr_el1 now points somewhere in the middle of the process function (wherever the interrupt)
// originally happened. And sp now points at the bottom of the task's original stack
//
// 0xXXXXXXXX +----------------------+
//            | TaskStruct           |
//            +----------------------+
//            |                      |
//            | Stack (grows up)     |
//            +----------------------+
//            | ProcessState         |
// 0xXXXX1000 +----------------------+
//            |         ...          |
// 0xYYYYYYYY +----------------------+
//            | TaskStruct           |
//            +----------------------+
//            |                      |
//            +----------------------+
//            | Stack (interrupt)    |
//            +----------------------+
//            | Task saved registers |
//            +----------------------+
//            | Stack (grows up)     |
//            +----------------------+
//            | ProcessState         |
// 0xYYYY1000 +----------------------+
//
// The eret instruction is executed, using the saved elr_el1 register to jump back to whatever the first task was doing

namespace
{
    // SPSR_EL1 bits - See section C5.2.18 in the ARMv8 manual
    constexpr uint64_t PSRModeEL0tC = 0x0000'0000;

    // Expected to match what is put on the stack via the kernel_entry macro in the exception handler so it can
    // "restore" the processor state we want
    struct ProcessState
    {
        // #TODO: Investigate improvements to eliminate lint tag
        // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays, hicpp-avoid-c-arrays, modernize-avoid-c-arrays, cppcoreguidelines-avoid-magic-numbers, readability-magic-numbers)
        uint64_t Registers[31] = {0U};
        uint64_t StackPointer = 0U;
        uint64_t ProgramCounter = 0U;
        uint64_t ProcessorState = 0U;
    };

    static_assert(offsetof(Scheduler::TaskStruct, Context) == TASK_STRUCT_CONTEXT_OFFSET, "Unexpected offset of context in task struct");
}

extern "C"
{
    /**
     * Return to the newly forked task (Defined in ExceptionVectors.S)
     */
    extern void ret_from_fork();

    /**
     * Switch the CPU from running the previous task to the next task
     * 
     * @param apPrev The previously running task
     * @param apNext The new task to resume
     */
    extern void cpu_switch_to(Scheduler::TaskStruct* apPrev, Scheduler::TaskStruct* apNext);
}

namespace
{
    constexpr auto TimerTickMSC = 200; // tick every 200ms

    constexpr auto ThreadSizeC = 4096; // 4k stack size (#TODO: Pull from page size?)
    constexpr auto NumberOfTasksC = 64U;

    // #TODO: We'll want something better to avoid the lint tag
    // NOLINTBEGIN(cppcoreguidelines-avoid-non-const-global-variables)

    Scheduler::TaskStruct InitTask; // task running kernel init
    Scheduler::TaskStruct* pCurrentTask = &InitTask;

    // #TODO: Convert to std::array when we have it to remove lint
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,hicpp-avoid-c-arrays,modernize-avoid-c-arrays)
    Scheduler::TaskStruct* Tasks[NumberOfTasksC] = { &InitTask, nullptr };

    auto NumberOfTasks = 1;

    // NOLINTEND(cppcoreguidelines-avoid-non-const-global-variables)

    /**
     * Enable scheduler preemption in the current task
     */
    void PreemptEnable()
    {
        // #TODO: Assert/error when we have it
        --pCurrentTask->PreemptCount;
    }

    /**
     * Disable scheduler preemption in the current task
     */
    void PreemptDisable()
    {
        ++pCurrentTask->PreemptCount;
    }

    /**
     * Helper to disable scheduler preempting in the current scope
     */
    struct DisablePreemptingInScope
    {
        /**
         * Constructor - disables scheduler preempting
         */
        [[nodiscard]] DisablePreemptingInScope()
        {
            PreemptDisable();
        }

        /**
         * Destructor - reenables scheduler preempting
         */
        ~DisablePreemptingInScope()
        {
            PreemptEnable();
        }

        // Disable copying/moving
        DisablePreemptingInScope(const DisablePreemptingInScope&) = delete;
        DisablePreemptingInScope(DisablePreemptingInScope&&) = delete;
        DisablePreemptingInScope& operator=(const DisablePreemptingInScope&) = delete;
        DisablePreemptingInScope& operator=(DisablePreemptingInScope&&) = delete;
    };

    /**
     * Switch from running the current task to the next task
     * 
     * @param apNextTask The next task to run
     */
    void SwitchTo(Scheduler::TaskStruct* const apNextTask)
    {
        if (pCurrentTask == apNextTask)
        {
            return;
        }
        auto* const pprevTask = pCurrentTask;
        pCurrentTask = apNextTask;
        MemoryManager::SetPageGlobalDirectory(pCurrentTask->MemoryState.PageGlobalDirectory);
        cpu_switch_to(pprevTask, apNextTask);
    }

    /**
     * Find and resume a running task
     */
    void ScheduleImpl()
    {
        // Make sure we don't get called while we're in the middle of picking a task
        DisablePreemptingInScope const disablePreempt;

        auto foundTask = false;
        auto taskToResume = 0U;
        while (!foundTask)
        {
            // Find the task with the largest counter value (i.e. the one that has the highest priority that has
            // not run in a while)
            auto largestCounter = -1LL;
            taskToResume = 0;
            for (auto curTask = 0U; curTask < NumberOfTasksC; ++curTask)
            {
                // #TODO: Can hopefully remove lint tag when we get std::array
                auto const* const ptask = Tasks[curTask]; // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
                if ((ptask != nullptr) && (ptask->State == Scheduler::TaskState::Running) && (ptask->Counter > largestCounter))
                {
                    largestCounter = ptask->Counter;
                    taskToResume = curTask;
                }
            }
            // If we don't find any running task with a non-zero counter, then we need to go and reset all their
            // counters according to their priority
            foundTask = (largestCounter > 0);
            if (!foundTask)
            {
                for (auto* const pcurTask : Tasks)
                {
                    if (pcurTask != nullptr)
                    {
                        // Increment the counter by the priority, ensuring that we don't go above 2 * priority
                        // So the longer the task has been waiting, the higher the counter should be.
                        pcurTask->Counter = (pcurTask->Counter / 1) + pcurTask->Priority;
                    }
                }
            }
            // If at least one task is running, then we should only loop around once. If all tasks are not running
            // then we keep looping until a task changes its state to running again (i.e. via an interrupt)
        }
        // #TODO: Can hopefully remove lint tag when we get std::array
        SwitchTo(Tasks[taskToResume]); // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
    }

    /**
     * Triggered by the timer interrupt to schedule a new task
     * 
     * @param apParam The parameter registered for our callback
     */
    void TimerTick(void const* const /*apParam*/)
    {
        // Only switch task if the counter has run out and it hasn't been blocked
        --pCurrentTask->Counter;
        if ((pCurrentTask->Counter > 0) || (pCurrentTask->PreemptCount > 0))
        {
            return;
        }
        pCurrentTask->Counter = 0;

        // Interrupts are disabled while handing one, so re-enable them for the schedule call because some tasks
        // might be waiting from an interrupt and we want them to be able to get them while the scheduler is trying
        // to find a task (otherwise we might just have all tasks waiting for interrupts and loop forever without
        // finding a task to run)
        enable_irq();

        ScheduleImpl();

        // And re-disable them before returning to the interrupt handler (which will deal with re-enabling them later)
        disable_irq();
    }

    /**
     * Extract the state memory from the stack for the given task
     * 
     * @param apTask Task to get the state for
     * @return The process state for the task
     */
    void* GetTargetStateMemoryForTask(Scheduler::TaskStruct const* const apTask)
    {
        const auto state = std::bit_cast<uintptr_t>(apTask) + ThreadSizeC - sizeof(ProcessState);
        return std::bit_cast<void*>(state);
    }
}

extern "C"
{
    /**
     * Called by assembly to finish any work up before staring the process call
     */
    void schedule_tail()
    {
        PreemptEnable();
    }
}

namespace Scheduler
{
    void InitTimer()
    {
        // We're using a local timer instead of the global timer because it works on both QEMU and real hardware
        //LocalTimer::RegisterCallback(TimerTickMSC, TimerTick, nullptr);
        // #TODO: Local timer cannot be used until we figure out how to get the local peripheral addresses mapped in
        // the mmu. Since they start at 0x4000'0000 they are out of range of our current single page mmu setup.
        Timer::RegisterCallback(TimerTickMSC, TimerTick, nullptr);
    }

    void Schedule()
    {
        pCurrentTask->Counter = 0;
        ScheduleImpl();
    }

    int CopyProcess(uint32_t const aCloneFlags, ProcessFunctionPtr const apProcessFn, void const* const apParam)
    {
        // Make sure we don't get preempted in the middle of making a new task
        DisablePreemptingInScope const disablePreempt;

        auto* const pmemory = MemoryManager::AllocateKernelPage();
        if (pmemory == nullptr)
        {
            return -1;
        }

        // #TODO: We'll want proper ownership figured out
        auto* const pnewTask = new (pmemory) TaskStruct{}; // NOLINT(cppcoreguidelines-owning-memory)

        auto* const puninitializedState = GetTargetStateMemoryForTask(pnewTask);
        // #TODO: We'll want proper ownership figured out
        auto* const pnewState = new (puninitializedState) ProcessState{}; // NOLINT(cppcoreguidelines-owning-memory)

        if ((aCloneFlags & CreationFlags::KernelThreadC) == CreationFlags::KernelThreadC)
        {
            pnewTask->Context.x19 = std::bit_cast<uint64_t>(apProcessFn);
            pnewTask->Context.x20 = std::bit_cast<uint64_t>(apParam);
        }
        else
        {
            // extract and clone the current processor state
            auto* const psourceState = std::bit_cast<ProcessState*>(GetTargetStateMemoryForTask(pCurrentTask));
            *pnewState = *psourceState;
            pnewState->Registers[0] = 0; // make sure ret_from_fork knows this is the new user process
            MemoryManager::CopyVirtualMemory(*pnewTask, *pCurrentTask);
        }

        pnewTask->Flags = aCloneFlags;
        pnewTask->Priority = pCurrentTask->Priority;
        pnewTask->Counter = pnewTask->Priority;
        pnewTask->PreemptCount = 1; // disable preemption until schedule_tail

        pnewTask->Context.pc = std::bit_cast<uint64_t>(&ret_from_fork);
        pnewTask->Context.sp = std::bit_cast<uint64_t>(pnewState);
        auto const processID = NumberOfTasks++;
        // #TODO: Can likely clean up lint tag when we get std::array
        Tasks[processID] = pnewTask; // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
        return processID;
    }

    // #TODO: Need better parameter types to avoid bugprone API. Maybe a span for start + size
    bool MoveToUserMode(void const* const apStart, std::size_t const aSize, uintptr_t const aPC) // NOLINT(bugprone-easily-swappable-parameters)
    {
        // We expect the state to have been constructed by CopyProcess before getting here
        auto* const pstate = std::bit_cast<ProcessState*>(GetTargetStateMemoryForTask(pCurrentTask));

        pstate->ProgramCounter = aPC;
        pstate->ProcessorState = PSRModeEL0tC;
        // We'll be basically reserving two pages of memory for the process for now. One for the code, and one for the
        // stack. The stack pointer won't be pre-allocated, as we'll allow the interrupt to trap access and map the
        // page for us, hence why we can just blindly set StackPointer here.
        pstate->StackPointer = 2 * MemoryManager::PageSize;

        auto* const pcodePage = MemoryManager::AllocateUserPage(*pCurrentTask, VirtualPtr{});
        if (pcodePage == nullptr)
        {
            pstate->~ProcessState();
            return false;
        }
        memcpy(pcodePage, apStart, aSize);
        MemoryManager::SetPageGlobalDirectory(pCurrentTask->MemoryState.PageGlobalDirectory);
        return true;
    }

    void ExitProcess()
    {
        {
            // Make sure we don't get preempted in the middle of cleaning up the task
            DisablePreemptingInScope const disablePreempt;

            // Flag the task as a zombie so it isn't rescheduled
            for (auto* const pcurTask : Tasks)
            {
                if (pcurTask == pCurrentTask)
                {
                    pcurTask->State = TaskState::Zombie;
                    break;
                }
            }
        }
        // Won't ever return because a new task will be scheduled and this one is now flagged as a zombie
        Schedule();
    }

    TaskStruct& GetCurrentTask()
    {
        return *pCurrentTask;
    }
}