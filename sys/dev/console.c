#include <lib/debug.h>
#include <lib/spinlock.h>
#include <lib/string.h>
#include <lib/types.h>

#include <dev/console.h>
#include <dev/kbd.h>
#include <dev/serial.h>
#include <dev/video.h>

struct {
	spinlock_t lk;

	char buf[CONSOLE_BUFFER_SIZE];
	uint32_t rpos, wpos;
	bool kbd_enabled;
} cons;

void
cons_init()
{
	memset(&cons, 0x0, sizeof(cons));

	spinlock_init(&cons.lk);

	cons.kbd_enabled = FALSE;

	serial_init();
	video_init();
	kbd_init();
}

void
cons_intr(int (*proc)(void))
{
	int c;

	while ((c = (*proc)()) != -1) {
		if (c == 0)
			continue;
		spinlock_acquire(&cons.lk);
		cons.buf[cons.wpos++] = c;
		if (cons.wpos == CONSOLE_BUFFER_SIZE)
			cons.wpos = 0;
		spinlock_release(&cons.lk);
	}
}

int
cons_getc(void)
{
	int c;

	// poll for any pending input characters,
	// so that this function works even when interrupts are disabled
	// (e.g., when called from the kernel monitor).
#ifdef SERIAL_DEBUG
	serial_intr();
#endif
	kbd_intr();

	// grab the next character from the input buffer.
	spinlock_acquire(&cons.lk);
	if (cons.rpos != cons.wpos) {
		c = cons.buf[cons.rpos++];
		if (cons.rpos == CONSOLE_BUFFER_SIZE)
			cons.rpos = 0;
		spinlock_release(&cons.lk);
		return c;
	}
	spinlock_release(&cons.lk);
	return 0;
}

void
cons_putc(char c)
{
#ifdef SERIAL_DEBUG
	serial_putc(c);
#endif
	video_putc(c);
}
