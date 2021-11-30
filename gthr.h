#ifndef __GTHR_H
#define __GTHR_H

#include <stdint.h>
#include <time.h>

#ifndef GT_PREEMPTIVE
#define GT_PREEMPTIVE 1
#endif

enum
{
    // Maximum number of threads, used as array size for gttbl
    MaxGThreads = 10,
    // Size of stack of each thread
    StackSize = 0x400000,
    // Time period of Timer
    TimePeriod = 10,
};

// Thread state
typedef enum
{
    Unused,
    Running,
    Ready,
    Blocked,
    Suspended,
} gt_thread_state_t;

struct gt_context_t
{
    struct gt_regs
    {
        // Saved context, switched by gt_*swtch.S (see for detail)
        uint64_t rsp;
        uint64_t rbp;
#if (GT_PREEMPTIVE == 0)
        uint64_t r15;
        uint64_t r14;
        uint64_t r13;
        uint64_t r12;
        uint64_t rbx;
#endif
    } regs;

    // Process state
    gt_thread_state_t thread_state;
    // Thread id
    unsigned int tid;
    // Thread name
    const char *name;
    // Thread arg
    void *arg;
};

/**
 * Initialize gttbl
 */
void gt_init(void);
/**
 * Create new anonymus thread
 * 
 * @param t_run function to run on thread
 * @return int thread id
 */
int gt_go(void (*t_run)(void), void *arg);
/**
 * Create new thread
 * 
 * @param t_run function to run on thread
 * @param name thread name
 * @return int thread id
 */
int gt_go_name(void (*t_run)(void), const char *name, void *arg);
/**
 * Terminate current thread
 */
void gt_stop(void);
/**
 * yield and switch to another thread
 * 
 * @return int yield status (0 - failure, 1 - success)
 */
int gt_yield(void);
/**
 * Start scheduler, wait for all tasks
 */
void gt_scheduler(void);
/**
 * Terminate thread 
 * 
 * @param t_ret UNUSED
 */
void gt_ret(int t_ret);

#if (GT_PREEMPTIVE == 0)
/**
 * Declaration from gtswtch.S
 * 
 * @param t_old swtich from
 * @param t_new switch to
 */
void gt_swtch(struct gt_regs *t_old, struct gt_regs *t_new);
#else
/**
 * Declaration from gtswtch.S
 * 
 * @param t_old swtich from
 * @param t_new switch to
 */
void gt_pree_swtch(struct gt_regs *t_old, struct gt_regs *t_new);
#endif

/**
 * Uninterruptible sleep
 * 
 * @param sec secunds
 * @param nanosec nanosecunds
 * @return int status (0 - success, -1 - failure)
 */
int uninterruptibleNanoSleep(time_t sec, long nanosec);
/**
 * Get current tid
 */
unsigned int gt_gettid();
/**
 * Get current name
 */
const char *gt_getname();
/**
 * Get current arg
 */
void *gt_getarg();
/**
 * Write task list to buffer
 * 
 * @param buffer to what will be written
 */
void gt_task_list(char *buffer);
/**
 * Change thread state to suspend
 * 
 * @param tid thread id
 */
void gt_suspend(unsigned int tid);
/**
 * Change thread state to ready
 * 
 * @param tid thread id
 */
void gt_resume(unsigned int tid);
#endif // __GTHR_H
