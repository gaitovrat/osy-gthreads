#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

#include "gthr.h"

#define COUNT 1000

// Task Hello
static void prvTaskHello(void)
{
    while (1)
    {
        printf("Hello world!\n");
        gt_task_delay(100);
    }
}


// Task "Ring"
static void prvTaskRing(void)
{
    int tid = *(int *)gt_getarg();
    while (1)
    {
    	gt_task_delay(500);
        printf("Ring! Ring! Ring!\n");
        gt_resume(tid);
    }
}


// Task "Sleep"
static void prvTaskSleep(void)
{
    while (1)
    {
        gt_suspend(gt_gettid());
        printf("Please do not wake up!\n");
    }
}

void list(void)
{
    char buffer[10000];

    while (1)
    {
    	gt_task_delay(200);
		gt_task_list(buffer);
		fprintf(stderr, "%s", buffer);
    }
}

void f(void)
{
	int count = 0;

	while (1)
	{
		printf("%s %d\n", gt_getname(), count++);
		uninterruptibleNanoSleep(0, 1000000);
	}
}

int main(void)
{
    gt_init();
    gt_go_name(f, "F1", NULL, 4);
    gt_go_name(f, "F2", NULL, 1);
    gt_go_name(prvTaskHello, "Hello", NULL, 1);
    int tid = gt_go_name(prvTaskSleep, "Sleep", NULL, 1);
    gt_go_name(prvTaskRing, "Ring", &tid, 1);
    gt_go(list, NULL, 1);
    gt_scheduler();

    printf("Threads finished\n");
}
