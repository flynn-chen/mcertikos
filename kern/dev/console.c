#include <lib/string.h>
#include <lib/types.h>
#include <lib/debug.h>
#include <lib/spinlock.h>
#include <lib/reentrant_lock.h>

#include "video.h"
#include "console.h"
#include "serial.h"
#include "keyboard.h"

#define BUFLEN 1024
static char linebuf[BUFLEN];

spinlock_t put_lock;   // lock when putting characters to the output ports (serial and video)
spinlock_t write_lock; // lock when writing to the console buffer, aka reading input from serial or keyboard
spinlock_t print_lock; // lock when calling readline

struct
{
    char buf[CONSOLE_BUFFER_SIZE];
    uint32_t rpos, wpos;
} cons;

void cons_init()
{
    memset(&cons, 0x0, sizeof(cons));
    serial_init();
    video_init();
    spinlock_init(&put_lock);
    spinlock_init(&write_lock);
    spinlock_init(&print_lock);
}

void cons_intr(int (*proc)(void))
{
    int c;

    spinlock_acquire(&write_lock);
    while ((c = (*proc)()) != -1)
    {
        if (c == 0)
            continue;
        cons.buf[cons.wpos++] = c;
        if (cons.wpos == CONSOLE_BUFFER_SIZE)
            cons.wpos = 0;
    }
    spinlock_release(&write_lock);
}

/**
 * This is only called by getchar, which is only called by readline.
 * So ensuring safety of readline ensures the safety of cons_getc
*/
char cons_getc(void)
{
    int c;

    // poll for any pending input characters,
    // so that this function works even when interrupts are disabled
    // (e.g., when called from the kernel monitor).
    serial_intr();
    keyboard_intr();

    // grab the next character from the input buffer.
    if (cons.rpos != cons.wpos)
    {
        c = cons.buf[cons.rpos++];
        if (cons.rpos == CONSOLE_BUFFER_SIZE)
            cons.rpos = 0;
        return c;
    }
    return 0;
}

/**
 * Random notes:
 *  - dprintf is surfaced to anyone, we can't control the people who call it
 *  - readline is called by the kernel monitor to get input from console
 *    (but could theoretically be called by anyone)
 * 
 * Desired behaviors:
 *  - while calling readline, no one else can call dprintf or readline
 *  - while calling dprintf, no one else can call dprintf or readline
 * 
 * Lock based on Desired Behaviors
 * 1. readline lock: prevent other processes from reading the prompt.
 * 2. cons_putc lock: only one proc can put to the console at a time.
*/

/**
 * This is called by cputs in dprintf.c, as well as readline in this file.
 * We need to ensure that while calling readline, no one else is allowed to print.
*/
void cons_putc(char c)
{
    /**
     *  try to acquire the print_lock
     *      if success, print, and release    
    */
    // TODO: if user is ever allowed to call putchar, think about reintroducing reentrantlock
    // reentrantlock_acquire(&print_lock);
    spinlock_acquire(&put_lock);
    serial_putc(c);
    video_putc(c);
    spinlock_release(&put_lock);
    // reentrantlock_release(&print_lock);
}

char getchar(void)
{
    char c;

    while ((c = cons_getc()) == 0)
        /* do nothing */;
    return c;
}

void putchar(char c)
{
    cons_putc(c);
}

char *readline(const char *prompt)
{
    spinlock_acquire(&print_lock);
    int i;
    char c;

    if (prompt != NULL)
        dprintf("%s", prompt);

    i = 0;
    while (1)
    {
        c = getchar();
        if (c < 0)
        {
            spinlock_release(&print_lock);
            dprintf("read error: %e\n", c);
            return NULL;
        }
        else if ((c == '\b' || c == '\x7f') && i > 0)
        {
            putchar('\b');
            i--;
        }
        else if (c >= ' ' && i < BUFLEN - 1)
        {
            putchar(c);
            linebuf[i++] = c;
        }
        else if (c == '\n' || c == '\r')
        {
            putchar('\n');
            linebuf[i] = 0;
            spinlock_release(&print_lock);
            return linebuf;
        }
    }
}

/**
 * Broad overview of our approach:
 *  - We have one print_lock governing sequential cons_putc requests
 *      - readline and dprintf both acquire this lock before running
 *  - We have one put_lock to ensure the serial and video outputs occur in the same order
 *  - We have one write_lock that ensures only one process can write to the cons.buf at a time
 * 
 * Questions:
 *  - Is it okay to extern a lock from console to share it with dprintf?
 *      - If not, how to lock dprintf? How can we ensure the ordering of sequential cons_putc?
 * 
 *  - If there's a KERNEL_PANIC that occurs when a lock is already acquired (e.g. during readline or during dprintf),
 *    how can it get the lock in order to print the panic message?
 * 
 * 
 * - make sure you always release the lock
 * - don't acquire the lock more than once
 * - lib/reentrant_lock.c can be acquired multiple times (may not be needed though)
 *      - still must acquire the same number as release
 * 
 * 
 * 
 * Example error output:

[D] kern/trap/TSyscall/TSyscall.c:161: [D] kern/dev/lapic.c:117: [0] Retry to calibrate internal timer of LAPIC.
CPU 1: Process 1: Produced 3

[D] kern/trap/TSyscall/TSyscall.c:161: [D] kern/dev/lapic.c:120: CPU 2: [4] Retry to calibrate internal timer of LAPIC.
[W] kern/dev/lapic.c:125: Failed to calibrate internal timer of LAPCPU 1: Process 1: Produced 4
IC.

*/