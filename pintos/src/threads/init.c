
#include <debug.h>
#include <random.h>
#include <stdbool.h>
#include "stdio.h"
#include <stdint.h>
#include <string.h>

#include "../devices/gpio.h"
#include "../devices/framebuffer.h"
#include "../devices/serial.h"
#include "../devices/timer.h"
#include "../devices/video.h"
#include "interrupt.h"
#include "init.h"
#include "palloc.h"
#include "malloc.h"
#include "synch.h"
#include "thread.h"
#include "vaddr.h"

/* -ul: Maximum number of pages to put into palloc's user pool. */
static size_t user_page_limit = SIZE_MAX;

/* Tasks for the Threads. */
static struct semaphore task_sem;

static void hello_test(void *);
static void A();
static void B();
static void C();
static void command_line();
static void command_help();
static void command_ts();

struct wait_node {
    struct lock mutex;
    struct condition cv;
    bool done;
};

static struct wait_node sync_node;

static void t_wait(struct wait_node *wn);
static void t_exit(struct wait_node *wn);
static void cv_test(void *);


/*
 * kernel.c
 *
 *  Created on: Oct 22, 2014
 *      Author: jcyescas
 */

static void test_swi_interrupt() {
    unsigned short green = 0x7E0;
    SetForeColour(green);
    generate_swi_interrupt(); // Function defined in interrupts.s
}


/* Initializes the Operating System. The interruptions have to be disabled at entrance.
*
*  - Sets interruptions
*  - Sets the periodic timer
*  - Set the thread functionality
*/
void init() {

    /* Initializes ourselves as a thread so we can use locks,
      then enable console locking. */
    thread_init();

    /* Initializes the frame buffer and console. */
    framebuffer_init();
    serial_init();
    video_init();

    printf("\nOsOs Kernel Initializing");

    /* Initialize memory system. */
    palloc_init (user_page_limit);
    malloc_init ();

    /* Initializes the Interrupt System. */
    interrupts_init();
    timer_init();

    timer_msleep(5000000);

    /* Starts preemptive thread scheduling by enabling interrupts. */
    thread_start();

    printf("\nFinish booting.");

    /* Initialize the task_sem to coordinate the main thread with workers */

    sema_init(&task_sem, 0);

//    thread_create("Hello Test", PRI_MAX, &hello_test, NULL);
//    sema_down(&task_sem);
//    thread_create("A", 35, &A, NULL);
//    thread_create("B", 36, &B, NULL);
//    thread_create("C", 37, &C, NULL);
    thread_create("shell", PRI_MAX, &command_line, NULL);

    lock_init(&sync_node.mutex);
    cond_init(&sync_node.cv);
    sync_node.done = false;

    thread_create("CV Test", PRI_MAX, &cv_test, NULL);
    t_wait(&sync_node);

    printf("\nAll done.");
    thread_exit ();
}

static void hello_test(void *aux) {
    printf("\n");
    printf("Hello from OsOS\n");
    printf("\n");
    sema_up(&task_sem);
}

static void t_wait(struct wait_node *wn)
{
    lock_acquire(&wn->mutex);
    while (!wn->done) {
        cond_wait(&wn->cv, &wn->mutex);
    }
    lock_release(&wn->mutex);
}

static void t_exit(struct wait_node *wn)
{
    lock_acquire(&wn->mutex);
    wn->done = true;
    cond_signal(&wn->cv,&wn->mutex);
    lock_release(&wn->mutex);
}

static void cv_test(void *aux) {
    printf("\n");
    printf("Hello from CV Test\n");
    printf("\n");
    t_exit(&sync_node);
}

static void A(){
    int i = 0;
    for (i = 0; i < 100; ++i) {
        printf("AAAAAAAAAAAAAAAAAAAAAAAAAA\n");
    }
    sema_up(&task_sem);
}

static void B(){
    int i = 0;
    for (i = 0; i < 100; ++i) {
        printf("BBBBBBBBBBBBBBBBBBBBBBBBBB\n");
    }

    sema_up(&task_sem);
}

static void C(){
    int i = 0;
    for (i = 0; i < 100; ++i) {
        printf("CCCCCCCCCCCCCCCCCCCCCCCCCC\n");
    }

    sema_up(&task_sem);
}

static void command_line(){
    int i=0,j=0,k=0;
    char buf[100];
    memset(buf,0,100);
    char *ts = "ts";
    char *help = "help";
    char temp;
    printf("\r\n");
    while (1){
        temp =  uart_getc();
        uart_putc(temp);
        if (temp!='\r'){
            buf[i++] = temp;
        } else{
            printf("this is else++++++++++++++\n");
            j = strcmp(buf,ts);
            k = strcmp(buf,help);
            if (j==0){
                command_ts();
            } else if (k==0){
                command_help();
            } else{
                printf("wrong command\n");
            }
            i=0;
            memset(buf,0,100);
        }
    }
}

static void command_help(){
    printf("This is a help command\n");
    printf("ts ---------\n");
    printf("run ---------\n");
    printf("bg ---------\n");
}

static void command_ts(){
    int count = getNumberOfThreads();
    printf("There are %d threads\n",count);
    thread_print_stats();
    printf("This is a ts command\n");
}

