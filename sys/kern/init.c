#include <sys/channel.h>
#include <sys/console.h>
#include <sys/context.h>
#include <sys/debug.h>
#include <sys/intr.h>
#include <sys/mboot.h>
#include <sys/mem.h>
#include <sys/mmu.h>
#include <sys/pcpu.h>
#include <sys/proc.h>
#include <sys/sched.h>
#include <sys/spinlock.h>
#include <sys/slab.h>
#include <sys/string.h>
#include <sys/trap.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/x86.h>

#include <sys/virt/vmm.h>

#include <machine/kstack.h>
#include <machine/pmap.h>

#include <dev/disk.h>
#include <dev/kbd.h>
#include <dev/lapic.h>
#include <dev/pci.h>
#include <dev/tsc.h>
#include <dev/timer.h>

/*
 * Reserve memory for the bootstrap kernel stack on the boostrap processor core.
 */
uint8_t bsp_kstack[KSTACK_SIZE] gcc_aligned(KSTACK_SIZE);

static volatile bool all_cpus_ready = FALSE;
static volatile bool vm_ready = FALSE;
static volatile bool all_vdevs_ready = FALSE;

extern uint8_t _binary___obj_user_idle_idle_start[],
	_binary___obj_user_vdev_i8042_i8042_start[],
	_binary___obj_user_vdev_i8254_i8254_start[],
	_binary___obj_user_vdev_nvram_nvram_start[],
	_binary___obj_user_vdev_virtio_virtio_start[];

static uint8_t *vdev_binary[4] =
	{
		_binary___obj_user_vdev_nvram_nvram_start,
		_binary___obj_user_vdev_i8042_i8042_start,
		_binary___obj_user_vdev_i8254_i8254_start,
		_binary___obj_user_vdev_virtio_virtio_start
	};

volatile struct vm *vm = NULL;

static void kern_main_ap(void);

/*
 * The message is generated by http://ascii.mastervb.net/.
 */
static char *welcome_msg =
	"   ____          _   _ _  _____  ____  \n"
	"  / ___|___ _ __| |_(_) |/ / _ \\/ ___| \n"
	" | |   / _ \\ '__| __| | ' / | | \\___ \\ \n"
	" | |___  __/ |  | |_| | . \\ |_| |___) |\n"
	"  \\____\\___|_|   \\__|_|_|\\_\\___/|____/ \n";

/*
 * The main function of the kernel on BSP and is called by kern_init().
 */
static void
kern_main(void)
{
	struct pcpu *c;
	struct kstack *ap_kstack;
	int i;

	c = pcpu_cur();
	KERN_ASSERT(c != NULL && c->booted == TRUE);

	/* register trap handlers */
	trap_init_array(c);
	KERN_INFO("[BSP KERN] Register exception handlers ... ");
	trap_handler_register(T_GPFLT, gpf_handler);
	trap_handler_register(T_PGFLT, pgf_handler);
	trap_handler_register(T_SYSCALL, syscall_handler);
	/* use default handler to handle other exceptions */
	trap_handler_register(T_DIVIDE, default_exception_handler);
	trap_handler_register(T_DEBUG, default_exception_handler);
	trap_handler_register(T_NMI, default_exception_handler);
	trap_handler_register(T_BRKPT, default_exception_handler);
	trap_handler_register(T_OFLOW, default_exception_handler);
	trap_handler_register(T_BOUND, default_exception_handler);
	trap_handler_register(T_ILLOP, default_exception_handler);
	trap_handler_register(T_DEVICE, default_exception_handler);
	trap_handler_register(T_DBLFLT, default_exception_handler);
	trap_handler_register(T_COPROC, default_exception_handler);
	trap_handler_register(T_TSS, default_exception_handler);
	trap_handler_register(T_SEGNP, default_exception_handler);
	trap_handler_register(T_STACK, default_exception_handler);
	trap_handler_register(T_RES, default_exception_handler);
	trap_handler_register(T_FPERR, default_exception_handler);
	trap_handler_register(T_ALIGN, default_exception_handler);
	trap_handler_register(T_MCHK, default_exception_handler);
	trap_handler_register(T_SIMD, default_exception_handler);
	trap_handler_register(T_SECEV, default_exception_handler);
	KERN_INFO("done.\n");

	KERN_INFO("[BSP KERN] Register interrupt handlers ... ");
	trap_handler_register(T_IRQ0+IRQ_SPURIOUS, spurious_intr_handler);
	trap_handler_register(T_IRQ0+IRQ_TIMER, timer_intr_handler);
	trap_handler_register(T_IRQ0+IRQ_KBD, kbd_intr_handler);
	trap_handler_register(T_IPI0+IPI_RESCHED, ipi_resched_handler);
	disk_register_intr();
	KERN_INFO("done.\n");

	/* enable interrupts */
	KERN_INFO("[BSP KERN] Enable TIMER interrupt ... ");
	intr_enable(IRQ_TIMER, 0);
	KERN_INFO("done.\n");

	KERN_INFO("[BSP KERN] Enable KBD interrupt ... ");
	kbd_intenable();
	KERN_INFO("done.\n");

	KERN_INFO("[BSP KERN] Enable disk interrupt ... ");
	disk_intr_enable();
	KERN_INFO("done.\n");

	/* boot APs  */
	all_cpus_ready = FALSE;
	for (i = 1; i < pcpu_ncpu(); i++) {
		KERN_INFO("Boot CPU%d ... ", i);

		if ((ap_kstack = kstack_alloc()) == NULL) {
			KERN_DEBUG("Cannot allocate memory for "
				   "kernel stack.\n");
			KERN_INFO("failed.\n");
			continue;
		}
		ap_kstack->cpu_idx = i;

		pcpu_boot_ap(i, kern_main_ap, (uintptr_t) ap_kstack);
		while (pcpu_get_cpu(i)->booted == FALSE);

		KERN_INFO("done.\n");
	}
	all_cpus_ready = TRUE;

	/* create VM */
#ifndef __COMPCERT__
	vm = vmm_create_vm(800 * 1000 * 1000, 256 * 1024 * 1024);
#else
	vm = vmm_create_vm(800 * 1000 * 1000, 0, 256 * 1024 * 1024);
#endif
	KERN_ASSERT(vm != NULL);
	vm_ready = TRUE;

	/* launch VM */
	while (all_vdevs_ready != TRUE)
		pause();
	KERN_INFO("Start VM ...\n");
	vmm_run_vm((struct vm *) vm);

	KERN_PANIC("[BSP KERN] CertiKOS should not be here!\n");
}

static void
kern_main_ap(void)
{
	struct pcpu *c;
	int cpu_idx;

	struct proc *idle_proc, *vdev_proc;
	struct channel *in, *out;
	vid_t vid;
	int i;

	c = pcpu_cur();
	KERN_ASSERT(c != NULL && c->booted == FALSE);
	cpu_idx = pcpu_cpu_idx(c);

	/* register trap handlers */
	trap_init_array(c);
	KERN_INFO("[AP%d KERN] Register exception handlers ... ", cpu_idx);
	trap_handler_register(T_GPFLT, gpf_handler);
	trap_handler_register(T_PGFLT, pgf_handler);
	trap_handler_register(T_SYSCALL, syscall_handler);
	/* use default handler to handle other exceptions */
	trap_handler_register(T_DIVIDE, default_exception_handler);
	trap_handler_register(T_DEBUG, default_exception_handler);
	trap_handler_register(T_NMI, default_exception_handler);
	trap_handler_register(T_BRKPT, default_exception_handler);
	trap_handler_register(T_OFLOW, default_exception_handler);
	trap_handler_register(T_BOUND, default_exception_handler);
	trap_handler_register(T_ILLOP, default_exception_handler);
	trap_handler_register(T_DEVICE, default_exception_handler);
	trap_handler_register(T_DBLFLT, default_exception_handler);
	trap_handler_register(T_COPROC, default_exception_handler);
	trap_handler_register(T_TSS, default_exception_handler);
	trap_handler_register(T_SEGNP, default_exception_handler);
	trap_handler_register(T_STACK, default_exception_handler);
	trap_handler_register(T_RES, default_exception_handler);
	trap_handler_register(T_FPERR, default_exception_handler);
	trap_handler_register(T_ALIGN, default_exception_handler);
	trap_handler_register(T_MCHK, default_exception_handler);
	trap_handler_register(T_SIMD, default_exception_handler);
	trap_handler_register(T_SECEV, default_exception_handler);
	KERN_INFO("done.\n");

	KERN_INFO("[AP%d KERN] Register interrupt handlers ... ", cpu_idx);
	trap_handler_register(T_IRQ0+IRQ_SPURIOUS, spurious_intr_handler);
	trap_handler_register(T_IRQ0+IRQ_TIMER, timer_intr_handler);
	trap_handler_register(T_IRQ0+IRQ_KBD, kbd_intr_handler);
	trap_handler_register(T_IPI0+IPI_RESCHED, ipi_resched_handler);
	disk_register_intr();
	KERN_INFO("done.\n");

	/* enable interrupts */

	c->booted = TRUE;
	while (all_cpus_ready == FALSE)
		pause();

	/* create idle process */
	if ((idle_proc = proc_new(NULL, NULL)) == NULL)
		KERN_PANIC("Cannot create idle process on AP%d.\n", cpu_idx);
	proc_exec(idle_proc, c, (uintptr_t) _binary___obj_user_idle_idle_start);

	if (pcpu_cpu_idx(pcpu_cur()) != 1)
		goto enter_user;

	/* create virtual devices  */
	while (vm_ready != TRUE)
		pause();
	for (i = 0; i < 4; i++) {
		out = channel_alloc(sizeof(vdev_ack_t));
		KERN_ASSERT(out != NULL);
		in = channel_alloc(sizeof(vdev_req_t));
		KERN_ASSERT(in != NULL);
		vdev_proc = proc_new(NULL, NULL);
		vid = vdev_register_device((struct vm *) vm, vdev_proc, in, out);
		proc_exec(vdev_proc, pcpu_cur(), (uintptr_t) vdev_binary[i]);
		KERN_DEBUG("Attach process %d to vdev %d, in %d, out %d.\n",
			   vdev_proc->pid, vid,
			   channel_getid(in), channel_getid(out));
	}
	all_vdevs_ready = TRUE;

 enter_user:
	/* jump to userspace */
	KERN_INFO("[AP%d KERN] Go to userspace ... \n", cpu_idx);
	sched_lock(c);
	sched_resched(FALSE);

	KERN_PANIC("[AP%d KERN] CertiKOS should not be here.\n", cpu_idx);
}

/*
 * The C entry of the kernel on BSP and is called by start().
 */
void
kern_init(mboot_info_t *mbi)
{
	/*
	 * Clear BSS.
	 *
	 *           :              :
	 *           |              |
	 *       /   +--------------+ <-- end
	 *       |   |              |
	 *       |   :      SBZ     :
	 *       |   |              |
	 *       |   +--------------+ <-- bsp_kstack + KSTACK_SIZE
	 *      BSS  |    kstack    |
	 *       |   +--------------+ <-- bsp_kstack
	 *       |   |              |
	 *       |   :      SBZ     :
	 *       |   |              |
	 *       \   +--------------+ <-- edata
	 *           |              |
	 *           :              :
	 */
	extern uint8_t end[], edata[];
	struct kstack *kstack = (struct kstack *) bsp_kstack;
	memzero(edata, bsp_kstack - edata);
	memzero(bsp_kstack + KSTACK_SIZE, end - bsp_kstack - KSTACK_SIZE);

#ifdef __COMPCERT__
	/*
	 * XXX: CompCert may generate the code using XMM* registers even though
	 *      the source code contains no floating-point operations.
	 */
	ccomp_enable_sse();
#endif

	/*
	 * Initialize the console so that we can output debug messages to the
	 * screen and/or the serial port.
	 */
	cons_init();
	debug_init();
	KERN_INFO("%s\n", welcome_msg);

	/*
	 * Initialize the bootstrap kernel stack, i.e. loading the bootstrap
	 * GDT, TSS and IDT, etc.
	 */
	KERN_INFO("Initialize bootstrap kernel stack ... ");
	kstack_init(kstack);
	kstack->cpu_idx = 0;
	KERN_INFO("done.\n");

	/*
	 * Initialize kernel memory allocator.
	 */
	KERN_INFO("Initialize kernel memory allocator ... ");
	mem_init(mbi);
	KERN_INFO("done.\n");

	/*
	 * Initialize PCPU module.
	 */
	KERN_INFO("Initialize PCPU module ... ");
	pcpu_init();
	KERN_INFO("done.\n");
	pcpu_cur()->kstack = kstack;
	pcpu_cur()->booted = TRUE;
	pcpu_init_cpu(); /* CPU specific initielization */

	/*
	 * Initialize kernel page table.
	 */
	KERN_INFO("Initialize kernel page table ... ");
	pmap_init();
	KERN_INFO("done.\n");

	/*
	 * Initialize slab allocator.
	 */
	KERN_INFO("Initialize slab allocator ... ");
	if (kmem_cache_init()) {
		KERN_INFO("failed.\n");
		KERN_PANIC("Cannot initialize slab allocator.\n");
	}
	KERN_INFO("done.\n");

	/*
	 * Initialize i8253 timer.
	 * XXX: MUST be initialized before tsc_init().
	 */
	KERN_INFO("Initialize i8253 timer ... ");
	timer_hw_init();
	KERN_INFO("done.\n");

	/*
	 * Calibrate TSC.
	 * XXX: Must be initialized before lapic_init().
	 */
	KERN_INFO("Initialize TSC ... ");
	if (tsc_init()) {
		KERN_INFO("failed.\n");
		KERN_WARN("System time will be inaccurate.\n");
	}
	KERN_INFO("done.\n");

	/*
	 * Intialize interrupt system.
	 * XXX: lapic_init() is called in intr_init().
	 */
	KERN_INFO("Initialize the interrupt system ... ");
	intr_init();
	KERN_INFO("done.\n");

	/* Initialize process module. */
	KERN_INFO("Initialize process module ... ");
	proc_init();
	KERN_INFO("done.\n");

	/* Initialize channel module. */
	KERN_INFO("Initialize channel module ... ");
	channel_init();
	KERN_INFO("done.\n");

	/*
	 * Initialize virtual machine monitor module.
	 */
	if (strncmp(pcpu_cur()->arch_info.vendor, "AuthenticAMD", 20) == 0 ||
	    strncmp(pcpu_cur()->arch_info.vendor, "GenuineIntel", 20) == 0) {
		KERN_INFO("Initialize VMM ... ");
		if (vmm_init() != 0)
			KERN_INFO("failed.\n");
		else
			KERN_INFO("done.\n");
	}

	/*
	 * Initialize disk management module.
	 */
	KERN_INFO("Initialize disk management module ... ");
	if (disk_init()) {
		KERN_INFO("failed.\n");
		KERN_PANIC("Stop here.\n");
	}
	KERN_INFO("done.\n");

	/*
	 * Initialize PCI bus.
	 **/
	KERN_INFO("Initialize PCI ... \n");
	pci_init();
	KERN_INFO("done.\n");

	/* Start master kernel on BSP */
	KERN_INFO("Start kernel on BSP ... \n");
	kern_main();

	/* should not be here */
	KERN_PANIC("We should not be here.\n");
}

void
kern_init_ap(void (*f)(void))
{
	KERN_INFO("\n");

#ifdef __COMPCERT__
	/*
	 * XXX: CompCert may generate the code using XMM* registers even though
	 *      the source code contains no floating-point operations.
	 */
	ccomp_enable_sse();
#endif

	struct kstack *ks = kstack_get_stack();

	kstack_init(ks);
	pcpu_cur()->kstack = ks;
	pcpu_init_cpu(); /* CPU specific initielization */
	pmap_init();
	intr_init();

	f();	/* kern_main_ap() */
}
