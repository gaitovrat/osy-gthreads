#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

#include "gthr.h"

#define COUNT 100

void f(void) {
    int count = COUNT;
    int *arg = gt_getarg();
    
    while ( count-- )
    {
        if (arg != NULL)
        {
            gt_suspend(*arg);
        } else
        {
            printf("%s\n", gt_getname());
        }
        uninterruptibleNanoSleep( 0, 1000000 );
#if (GT_PREEMPTIVE == 0)
        gt_yield();
#endif
    }
}

void g(void) {
    while (1)
    {
        uninterruptibleNanoSleep( 0, 1000000 );
#if (GT_PREEMPTIVE == 0)
        gt_yield();
#endif
    }
}

void list(void)
{
    char buffer[10000];
    int size;
    int *arg = gt_getarg();

    while(1)
    {
        size = read(STDIN_FILENO, buffer, 1);
        if (size == -1 || (*buffer != '\n' && *buffer != 'c'))
        {
            continue;
        }

        if (*buffer == '\n')
        {
            gt_task_list(buffer);
            printf("%s", buffer);
        } else
        {
            gt_resume(*arg);
        }
    }
}
int main(void) 
{
    gt_init();      
    int tid1 = gt_go_name("F1", f, NULL);     
    gt_go_name("F2", f, &tid1);    
    gt_go(g, NULL);   
    gt_go_name("list", list, &tid1);
    gt_scheduler(); 

    printf( "Threads finished\n" );
}