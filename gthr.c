#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <sys/time.h>

#include "gthr.h"

// Statically allocated table for thread control
struct gt_context_t g_gttbl[MaxGThreads];
// Pointer to current thread
struct gt_context_t *g_gtcur;

#if (GT_PREEMPTIVE != 0)
/**
 * Handle signale
 * 
 * @param t_sig signal to handle
 */
void gt_sig_handle(int t_sig);

/**
 * Initialize SIGALRM
 */
void gt_sig_start(void)
{
    struct sigaction l_sig_act;
    bzero(&l_sig_act, sizeof(l_sig_act));
    l_sig_act.sa_handler = gt_sig_handle;

    sigaction(SIGALRM, &l_sig_act, NULL);
    ualarm(TimePeriod * 1000, TimePeriod * 1000);
}

/**
 * Unblock SIGALRM
 */
void gt_sig_reset(void)
{
    // Create signal set
    sigset_t l_set;
    // Clear it
    sigemptyset(&l_set);
    // Set signal (we use SIGALRM)
    sigaddset(&l_set, SIGALRM);

    // Fetch and change the signal mask
    sigprocmask(SIG_UNBLOCK, &l_set, NULL);
}

void gt_sig_handle(int t_sig)
{
    // Enable SIGALRM again
    gt_sig_reset();

    // .... manage timers here

    // Scheduler
    gt_yield();
}
#endif

void gt_init(void)
{
    // Initialize current thread with thread #0
    g_gtcur = &g_gttbl[0];
    // Set current to running
    g_gtcur->thread_state = Running;
    g_gtcur->tid = 0;
#if (GT_PREEMPTIVE != 0)
    gt_sig_start();
#endif
}

void gt_ret(int t_ret)
{
    // If not an initial thread
    if (g_gtcur != &g_gttbl[0])
    {
        // Set current thread as unused
        g_gtcur->thread_state = Unused;
        // Yield and make possible to switch to another thread
        gt_yield();
        // This code should never be reachable ... (if yes, returning function on stack was corrupted)
        assert(!"reachable");
    }
}

void gt_scheduler(void)
{
    // If initial thread, wait for other to terminate
    while (gt_yield())
    {
        if (GT_PREEMPTIVE)
        {
            // Idle, simulate CPU HW sleep
            usleep(TimePeriod * 1000);
        }
    }
}

int gt_yield(void)
{
    struct gt_context_t *p;
    struct gt_regs *l_old, *l_new;
    // Not ready processes
    int l_no_ready = 0;

    p = g_gtcur;
    // Iterate through g_gttbl[] until we find new thread in state Ready
    while (p->thread_state != Ready)
    {
        if (p->thread_state == Blocked || p->thread_state == Suspended)
        {
            // Number of Blocked and Suspend processes
            l_no_ready++;
        }

        // At the end rotate to the beginning
        if (++p == &g_gttbl[MaxGThreads])
        {
            p = &g_gttbl[0];
        }
        // Did not find any other Ready threads
        if (p == g_gtcur)
        {
            // No task ready, or task table empty
            return -l_no_ready;
        }
    }

    // Switch current to Ready and new thread found in previous loop to Running
    if (g_gtcur->thread_state == Running)
    {
        g_gtcur->thread_state = Ready;
    }
    p->thread_state = Running;
    // Prepare pointers to context of current (will become old)
    l_old = &g_gtcur->regs;
    // And new to new thread found in previous loop
    l_new = &p->regs;
    // Switch current indicator to new thread
    g_gtcur = p;
#if (GT_PREEMPTIVE != 0)
    // Perform context switch (assembly in gtswtch.S)
    gt_pree_swtch(l_old, l_new);
#else
    // {erform context switch (assembly in gtswtch.S)
    gt_swtch(l_old, l_new);
#endif
    return 1;
}

void gt_stop(void)
{
    gt_ret(0);
}

int gt_go(void (*t_run)(void))
{
    char *l_stack;
    struct gt_context_t *p;

    // Find an empty slot
    for (p = &g_gttbl[0];; p++)
    {
        // If we have reached the end, gttbl is full and we cannot create a new thread
        if (p == &g_gttbl[MaxGThreads])
        {
            return -1;
        }
        else if (p->thread_state == Unused)
        {
            // New slot was found
            break;
        }
    }
    // Allocate memory for stack of newly created thread
    l_stack = (char *)malloc(StackSize);
    if (!l_stack)
    {
        return -1;
    }

    // Put into the stack returning function gt_stop in case function calls return
    *(uint64_t *)&l_stack[StackSize - 8] = (uint64_t)gt_stop;
    // Put provided function as a main "run" function
    *(uint64_t *)&l_stack[StackSize - 16] = (uint64_t)t_run;
    // Set stack pointer
    p->regs.rsp = (uint64_t)&l_stack[StackSize - 16];
    // Set state
    p->thread_state = Ready;
    // Set tid
    p->tid = p - &g_gttbl[0];

    return 0;
}

int uninterruptibleNanoSleep(time_t t_sec, long t_nanosec)
{
#if 0
    struct timeval l_tv_cur, l_tv_limit;
    struct timeval l_tv_tmp = { t_sec, t_nanosec / 1000 };
    gettimeofday( &l_tv_cur, NULL );
    timeradd( &l_tv_cur, &l_tv_tmp, &l_tv_limit );
    while ( timercmp( &l_tv_cur, &l_tv_limit, <= ) )
    {
        timersub( &l_tv_limit, &l_tv_cur, &l_tv_tmp );
        struct timespec l_tout = { l_tv_tmp.tv_sec, l_tv_tmp.tv_usec * 1000 }, l_res;    
        if ( nanosleep( & l_tout, &l_res ) < 0 )
        {
            if ( errno != EINTR )
                return -1;
        }
        else
        {
            break;
        }
        gettimeofday( &l_tv_cur, NULL );
    }

#else
    struct timespec req;
    req.tv_sec = t_sec;
    req.tv_nsec = t_nanosec;

    do
    {
        if (0 != nanosleep(&req, &req))
        {
            if (errno != EINTR)
                return -1;
        }
        else
        {
            break;
        }
    } while (req.tv_sec > 0 || req.tv_nsec > 0);
#endif
    // Return success
    return 0;
}

size_t gt_gettid()
{
    return g_gtcur->tid;
}