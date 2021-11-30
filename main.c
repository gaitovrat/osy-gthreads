#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

#include "gthr.h"

#define COUNT 100

// Dummy function to simulate some thread work
void f(void)
{
    static int x;
    int count = COUNT;
    int id = ++x;

    while (count--)
    {
        printf("F Thread id: %d count: %d\n", id, count);
        uninterruptibleNanoSleep(0, 1000000);
#if (GT_PREEMPTIVE == 0)
        gt_yield();
#endif
    }
}

// Dummy function to simulate some thread work
void g(void)
{
    static int x = 0;
    int count = COUNT;
    int id = ++x;

    while (count--)
    {
        printf("G Thread id: %d count: %d\n", id, count);
        uninterruptibleNanoSleep(0, 1000000);
#if (GT_PREEMPTIVE == 0)
        gt_yield();
#endif
    }
}

int main(void)
{
    // Initialize threads, see gthr.c
    gt_init();
    // Set f() as first thread
    gt_go(f);
    // Set f() as second thread
    gt_go(f);
    // Set g() as third thread
    gt_go(g);
    // Set g() as fourth thread
    gt_go(g);
    // Wait until all threads terminate
    gt_scheduler();

    printf("Threads finished\n");
}
