#include "Scheduler.h"

#include <new>
#include "ARM/SchedulerDefines.h"
#include "IRQ.h"
#include "MemoryManager.h"
#include "Timer.h"

namespace
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
        Running
    };

    struct TaskStruct
    {
        CPUContext Context;
        TaskState State = TaskState::Running;
        int64_t Counter = 0; // decrements each timer tick. When reaches 0, another task will be scheduled
        int64_t Priority = 1; // copied to Counter when a task is scheduled, so higher priority will run for longer
        int64_t PreemptCount = 0; // If non-zero, task will not be preempted
    };

    static_assert(offsetof(TaskStruct, Context) == TASK_STRUCT_CONTEXT_OFFSET, "Unexpected offset of context in task struct");
}

extern "C"
{
    /**
     * Return to the newly created task
     */
    extern void ret_from_create();

    /**
     * Switch the CPU from running the previous task to the next task
     * 
     * @param apPrev The previously running task
     * @param apNext The new task to resume
     */
    extern void cpu_switch_to(TaskStruct* apPrev, TaskStruct* apNext);
}

namespace
{
    constexpr auto TimerTickMSC = 200; // tick every 200ms

    constexpr auto ThreadSizeC = 4096; // 4k stack size
    constexpr auto NumberOfTasksC = 64u;
    TaskStruct InitTask; // task running kernel init

    TaskStruct* pCurrentTask = &InitTask;
    TaskStruct* Tasks[NumberOfTasksC] = {&InitTask, nullptr};
    auto NumberOfTasks = 1;

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

        // Disable copying
        DisablePreemptingInScope(const DisablePreemptingInScope&) = delete;
        DisablePreemptingInScope& operator=(const DisablePreemptingInScope&) = delete;
    };

    /**
     * Switch from running the current task to the next task
     * 
     * @param apNextTask The next task to run
     */
    void SwitchTo(TaskStruct* const apNextTask)
    {
        if (pCurrentTask == apNextTask)
        {
            return;
        }
        const auto pprevTask = pCurrentTask;
        pCurrentTask = apNextTask;
        cpu_switch_to(pprevTask, apNextTask);
    }

    /**
     * Find and resume a running task
     */
    void ScheduleImpl()
    {
        // Make sure we don't get called while we're in the middle of picking a task
        DisablePreemptingInScope disablePreempt;

        auto foundTask = false;
        auto taskToResume = 0u;
        while (!foundTask)
        {
            // Find the task with the largest counter value (i.e. the one that has the highest priority that has
            // not run in a while)
            auto largestCounter = -1ll;
            taskToResume = 0;
            for (auto curTask = 0u; curTask < NumberOfTasksC; ++curTask)
            {
                const auto ptask = Tasks[curTask];
                if (ptask && (ptask->State == TaskState::Running) && (ptask->Counter > largestCounter))
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
                for (auto curTask = 0u; curTask < NumberOfTasksC; ++curTask)
                {
                    const auto ptask = Tasks[curTask];
                    if (ptask)
                    {
                        // Increment the counter by the priority, ensuring that we don't go above 2 * priority
                        // So the longer the task has been waiting, the higher the counter should be.
                        ptask->Counter = (ptask->Counter / 1) + ptask->Priority;
                    }
                }
            }
            // If at least one task is running, then we should only loop around once. If all tasks are not running
            // then we keep looping until a task changes its state to running again (i.e. via an interrupt)
        }
        SwitchTo(Tasks[taskToResume]);
    }

    /**
     * Triggered by the timer interrupt to schedule a new task
     * 
     * @param apParam The parameter registered for our callback
     * @return True to register another timer tick, false to stop timer ticking
     */
    bool TimerTick(const void* /*apParam*/)
    {
        // Only switch task if the counter has run out and it hasn't been blocked
        --pCurrentTask->Counter;
        if ((pCurrentTask->Counter > 0) || (pCurrentTask->PreemptCount > 0))
        {
            return true;
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

        return true;
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
        LocalTimer::RegisterCallback(TimerTickMSC, TimerTick, nullptr);
    }

    void Schedule()
    {
        pCurrentTask->Counter = 0;
        ScheduleImpl();
    }

    bool CreateProcess(ProcessFunctionPtr apProcessFn, const void* apParam)
    {
        // Make sure we don't get preempted in the middle of making a new task
        DisablePreemptingInScope disablePreempt;

        auto pmemory = reinterpret_cast<uint8_t*>(MemoryManager::GetFreePage());
        auto pnewTask = new (pmemory) TaskStruct{};
        if (pnewTask == nullptr)
        {
            return false;
        }
        pnewTask->Priority = pCurrentTask->Priority;
        pnewTask->Counter = pnewTask->Priority;
        pnewTask->PreemptCount = 1; // disable preemption until schedule_tail

        pnewTask->Context.x19 = reinterpret_cast<uint64_t>(apProcessFn);
        pnewTask->Context.x20 = reinterpret_cast<uint64_t>(apParam);
        pnewTask->Context.pc = reinterpret_cast<uint64_t>(ret_from_create);
        pnewTask->Context.sp = reinterpret_cast<uint64_t>(pmemory + ThreadSizeC);
        const auto processID = NumberOfTasks++;
        Tasks[processID] = pnewTask;
        return true;
    }
}