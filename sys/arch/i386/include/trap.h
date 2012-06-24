#ifndef _MACHINE_TRAP_H_
#define _MACHINE_TRAP_H_

#ifdef _KERN_

/* Trap numbers */

/* These are processor defined: */
#define T_DIVIDE	0	/* divide error */
#define T_DEBUG		1	/* debug exception */
#define T_NMI		2	/* non-maskable interrupt */
#define T_BRKPT		3	/* breakpoint */
#define T_OFLOW		4	/* overflow */
#define T_BOUND		5	/* bounds check */
#define T_ILLOP		6	/* illegal opcode */
#define T_DEVICE	7	/* device not available */
#define T_DBLFLT	8	/* double fault */
#define T_COPROC	9	/* reserved (not generated by recent processors) */
#define T_TSS		10	/* invalid task switch segment */
#define T_SEGNP		11	/* segment not present */
#define T_STACK		12	/* stack exception */
#define T_GPFLT		13	/* general protection fault */
#define T_PGFLT		14	/* page fault */
#define T_RES		15	/* reserved */
#define T_FPERR		16	/* floating point error */
#define T_ALIGN		17	/* aligment check */
#define T_MCHK		18	/* machine check */
#define T_SIMD		19	/* SIMD floating point exception */
#define T_SECEV		30	/* Security-sensitive event */

#define T_IRQ0		32	/* Legacy ISA hardware interrupts: IRQ0-15. */

/*
 * The rest are arbitrarily chosen, but with care not to overlap
 * processor defined exceptions or ISA hardware interrupt vectors.
 */
#define T_SYSCALL	48	/* System call */

/* We use these vectors to receive local per-CPU interrupts */
#define T_LTIMER	49	/* Local APIC timer interrupt */
#define T_LERROR	50	/* Local APIC error interrupt */
#define T_PERFCTR	51	/* Performance counter overflow interrupt */

#define T_DEFAULT	500	/* Unused trap vectors produce this value */
#define T_ICNT		501	/* Child process instruction count expired */

#define T_MAX		512

/* ISA hardware IRQ numbers. We receive these as (T_IRQ0 + IRQ_WHATEVER) */
#define IRQ_TIMER	0	/* 8253 Programmable Interval Timer (PIT) */
#define IRQ_KBD		1	/* Keyboard interrupt */
#define IRQ_SERIAL	4	/* Serial (COM) interrup */
#define IRQ_SPURIOUS	7	/* Spurious interrupt */
#define IRQ_MOUSE	12	/* Mouse interrupt */
#define IRQ_IDE		14	/* IDE disk controller interrupt */
#define IRQ_ERROR	19	/* */

#ifndef __ASSEMBLER__

#include <sys/debug.h>
#include <sys/gcc.h>
#include <sys/types.h>

typedef
struct pushregs {
	uint32_t edi;
	uint32_t esi;
	uint32_t ebp;
	uint32_t oesp;		/* Useless */
	uint32_t ebx;
	uint32_t edx;
	uint32_t ecx;
	uint32_t eax;
} pushregs;

typedef
struct tf_t {
	/* registers and other info we push manually in trapasm.S */
	pushregs regs;
	uint16_t es;		uint16_t padding_es;
	uint16_t ds;		uint16_t padding_ds;
	uint32_t trapno;

	/* format from here on determined by x86 hardware architecture */
	uint32_t err;
	uintptr_t eip;
	uint16_t cs;		uint16_t padding_cs;
	uint32_t eflags;

	/* rest included only when crossing rings, e.g., user to kernel */
	uintptr_t esp;
	uint16_t ss;		uint16_t padding_ss;
} tf_t;

void trap_return(tf_t *) gcc_noreturn;

static void gcc_inline
trap_dump(tf_t *tf)
{
	if (tf == NULL)
		return;

	uintptr_t base = (uintptr_t) tf;

	KERN_DEBUG("trapframe at %x\n", base);
	debug_lock();
	dprintf("\t%08x:\tedi:   \t\t%08x\n", &tf->regs.edi, tf->regs.edi);
	dprintf("\t%08x:\tesi:   \t\t%08x\n", &tf->regs.esi, tf->regs.esi);
	dprintf("\t%08x:\tebp:   \t\t%08x\n", &tf->regs.ebp, tf->regs.ebp);
	dprintf("\t%08x:\tesp:   \t\t%08x\n", &tf->regs.oesp, tf->regs.oesp);
	dprintf("\t%08x:\tebx:   \t\t%08x\n", &tf->regs.ebx, tf->regs.ebx);
	dprintf("\t%08x:\tedx:   \t\t%08x\n", &tf->regs.edx, tf->regs.edx);
	dprintf("\t%08x:\tecx:   \t\t%08x\n", &tf->regs.ecx, tf->regs.ecx);
	dprintf("\t%08x:\teax:   \t\t%08x\n", &tf->regs.eax, tf->regs.eax);
	/* dprintf("\t%08x:\tgs:    \t\t%08x\n", &tf->gs, tf->gs); */
	/* dprintf("\t%08x:\tfs:    \t\t%08x\n", &tf->fs, tf->fs); */
	dprintf("\t%08x:\tes:    \t\t%08x\n", &tf->es, tf->es);
	dprintf("\t%08x:\tds:    \t\t%08x\n", &tf->ds, tf->ds);
	dprintf("\t%08x:\ttrapno:\t\t%08x\n", &tf->trapno, tf->trapno);
	dprintf("\t%08x:\terr:   \t\t%08x\n", &tf->err, tf->err);
	dprintf("\t%08x:\teip:   \t\t%08x\n", &tf->eip, tf->eip);
	dprintf("\t%08x:\tcs:    \t\t%08x\n", &tf->cs, tf->cs);
	dprintf("\t%08x:\teflags:\t\t%08x\n", &tf->eflags, tf->eflags);
	dprintf("\t%08x:\tesp:   \t\t%08x\n", &tf->esp, tf->esp);
	dprintf("\t%08x:\tss:    \t\t%08x\n", &tf->ss, tf->ss);
	debug_unlock();
}

#endif /* !_ASSEMBLER__ */

#endif /* _KERN_ */

#endif /* !_MACHINE_TRAP_H_ */
