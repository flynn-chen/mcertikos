#include <sys/debug.h>
#include <sys/mmu.h>
#include <sys/pcpu.h>
#include <sys/proc.h>
#include <sys/string.h>
#include <sys/session.h>
#include <sys/syscall.h>
#include <sys/trap.h>
#include <sys/types.h>
#include <sys/vm.h>
#include <sys/x86.h>

#include <sys/virt/vmm.h>
#include <sys/virt/vmm_dev.h>

#include <dev/ahci.h>

#include <machine/pmap.h>

#ifdef DEBUG_SYSCALL

static char *syscall_name[MAX_SYSCALL_NR] =
	{
		[SYS_puts]		= "sys_puts",
		[SYS_getc]		= "sys_getc",
		[SYS_create_proc]	= "sys_create_proc",
		[SYS_run_proc]		= "sys_run_proc",
		[SYS_yield]		= "sys_yield",
		[SYS_getpid]		= "sys_getpid",
		[SYS_getppid]		= "sys_getppid",
		[SYS_session]		= "sys_session",
		[SYS_getsid]		= "sys_getsid",
		[SYS_getchid]		= "sys_getchid",
		[SYS_send]		= "sys_send",
		[SYS_recv]		= "sys_recv",
		[SYS_disk_op]		= "sys_disk_op",
		[SYS_disk_cap]		= "sys_disk_cap",
		[SYS_new_vm]		= "sys_new_vm",
		[SYS_run_vm]		= "sys_run_vm",
		[SYS_attach_vdev]	= "sys_attach_vdev",
		[SYS_detach_vdev]	= "sys_detach_vdev",
		[SYS_attach_port]	= "sys_attach_port",
		[SYS_detach_port]	= "sys_detach_port",
		[SYS_attach_irq]	= "sys_attach_irq",
		[SYS_detach_irq]	= "sys_detach_irq",
		[SYS_host_in]		= "sys_host_in",
		[SYS_host_out]		= "sys_host_out",
		[SYS_set_irq]		= "sys_set_irq",
		[SYS_guest_tsc]		= "sys_guest_tsc",
		[SYS_guest_cpufreq]	= "sys_guest_cpufreq",
		[SYS_guest_memsize]	= "sys_guest_memsize",
	};

static char *errno_name[MAX_ERROR_NR] =
	{
		[E_SUCC]		= "E_SUCC",
		[E_MEM]			= "E_MEM",
		[E_INVAL_CALLNR]	= "E_INVAL_CALLNR",
		[E_INVAL_CPU]		= "E_INVAL_CPU",
		[E_INVAL_SID]		= "E_INVAL_SID",
		[E_INVAL_ADDR]		= "E_INVAL_ADDR",
		[E_INVAL_PID]		= "E_INVAL_PID",
		[E_INVAL_CHID]		= "E_INVAL_CHID",
		[E_INVAL_VMID]		= "E_INVAL_VMID",
		[E_INVAL_VID]		= "E_INVAL_VID",
		[E_INVAL_IRQ]		= "E_INVAL_IRQ",
		[E_INVAL_MODE]		= "E_INVAL_MODE",
		[E_ATTACH]		= "E_ATTACH",
		[E_DETACH]		= "E_DETACH",
		[E_IOPORT]		= "E_IOPORT",
		[E_PIC]			= "E_PIC",
		[E_DEV_SYNC]		= "E_DEV_SYNC",
		[E_DEV_RDY]		= "E_DEV_RDY",
		[E_SEND]		= "E_SEND",
		[E_RECV]		= "E_RECV",
		[E_DISK_OP]		= "E_DISK_OP",
	};

#define SYSCALL_DEBUG(fmt, ...)				\
	do {						\
		KERN_DEBUG("CPU%d:PID%d: "fmt,		\
			   pcpu_cpu_idx(pcpu_cur()),	\
			   proc_cur()->pid,		\
			   ##__VA_ARGS__);		\
	} while (0)

#define NR_DEBUG(nr, fmt, ...)						\
	do {								\
		KERN_DEBUG("%s(nr %d, CPU%d, PID%d) "fmt,		\
			   (0 <= nr && nr < MAX_SYSCALL_NR) ?		\
			   syscall_name[nr] : "unknown",		\
			   nr,						\
			   pcpu_cpu_idx(pcpu_cur()),			\
			   proc_cur()->pid,				\
			   ##__VA_ARGS__);				\
	} while (0)

#define ERRNO_STR(errno)						\
	((errno) >= MAX_ERROR_NR ? "unknown" : errno_name[(errno)])

#else

#define SYSCALL_DEBUG(fmt...)			\
	do {					\
	} while (0)

#define NR_DEBUG(fmt...)			\
	do {					\
	} while (0)

#endif

extern uint8_t _binary___obj_user_vdev_i8042_i8042_start[],
	_binary___obj_user_vdev_i8254_i8254_start[],
	_binary___obj_user_vdev_nvram_nvram_start[],
	_binary___obj_user_vdev_virtio_virtio_start[];

#define ELF_8042	((uintptr_t) _binary___obj_user_vdev_i8042_i8042_start)
#define ELF_8254	((uintptr_t) _binary___obj_user_vdev_i8254_i8254_start)
#define ELF_NVRAM	((uintptr_t) _binary___obj_user_vdev_nvram_nvram_start)
#define ELF_VIRTIO	((uintptr_t) _binary___obj_user_vdev_virtio_virtio_start)

static uintptr_t vdev_elf_addr[MAX_VDEV] =
	{
		[VDEV_8042]	= ELF_8042,
		[VDEV_8254]	= ELF_8254,
		[VDEV_NVRAM]	= ELF_NVRAM,
		[VDEV_VIRTIO]	= ELF_VIRTIO,
	};

/*
 * Transfer data between the kernel linear address space and the user linear
 * address space.
 *
 * @param ka   the start linear address of the kernel linear address space
 * @param (pmap, la)
 *             the start linear address of the user linear address space
 * @param size how many bytes is transfered
 *
 * @return the number of bytes that are actually transfered
 */
static size_t
rw_user(uintptr_t ka, pmap_t *pmap, uintptr_t la, size_t size, int write)
{
	KERN_ASSERT(ka + size <= VM_USERLO);
	KERN_ASSERT(pmap != NULL);

	if (la < VM_USERLO || VM_USERLO - size > la)
		return 0;

	if (write)
		return pmap_copy(pmap, la, pmap_kern, ka, size);
	else
		return pmap_copy(pmap_kern, ka, pmap, la, size);
}

#define copy_from_user(ka, pmap, la, size)	\
	rw_user((ka), (pmap), (la), (size), 0)

#define copy_to_user(pmap, la, ka, size)	\
	rw_user((ka), (pmap), (la), (size), 1)

static int
sys_puts(uintptr_t str_la)
{
	struct proc *p = proc_cur();

	if (copy_from_user((uintptr_t) p->sys_buf,
			   p->pmap, str_la, PAGESIZE) == NULL)
		return E_MEM;

	((char *) (p->sys_buf))[PAGESIZE - 1] = '\0';
	KERN_INFO("%s", (char *) p->sys_buf);

	return E_SUCC;
}

static int
sys_getc(uintptr_t buf_la)
{
	struct proc *p = proc_cur();
	char c;

	c = getchar();
	if (copy_to_user(p->pmap, buf_la, (uintptr_t) &c, sizeof(char)) == NULL)
		return E_MEM;
	else
		return E_SUCC;
}

static int
sys_create_proc(uintptr_t binary_la, uintptr_t pid_la)
{
	struct proc *new_p, *cur_p;;

	/* if (!(VM_USERLO <= binary_la && */
	/*       binary_la + sizeof(uintptr_t) <= VM_USERHI)) */
	/* 	return E_INVAL_ADDR; */
	if (!(VM_USERLO <= pid_la && pid_la + sizeof(pid_t) <= VM_USERHI))
		return E_INVAL_ADDR;

	if (unlikely((cur_p = proc_cur()) == NULL))
		return E_INVAL_PID;

	if ((new_p = proc_new(binary_la, cur_p->session)) == NULL)
		return E_INVAL_PID;

	if (copy_to_user(cur_p->pmap, pid_la,
			 (uintptr_t) &new_p->pid, sizeof(pid_t)) == NULL)
		return E_MEM;

	return E_SUCC;
}

static int
sys_run_proc(pid_t pid, uint32_t cpu_idx)
{
	struct pcpu *c;
	struct proc *p;

	if (cpu_idx >= pcpu_ncpu())
		return E_INVAL_CPU;
	c = &pcpu[cpu_idx];

	if ((p = proc_pid2proc(pid)) == NULL || p->state != PROC_INITED)
		return E_INVAL_PID;

	proc_run(c, p);
	return E_SUCC;
}

static int
sys_yield(void)
{
	proc_yield();
	return E_SUCC;
}

static int
sys_getpid(uintptr_t pid_la)
{
	if (pid_la < VM_USERLO || pid_la + sizeof(pid_t) > VM_USERHI)
		return E_INVAL_ADDR;

	struct proc *p = proc_cur();
	KERN_ASSERT(p != NULL);

	if (copy_to_user(p->pmap, pid_la,
			 (uintptr_t) &p->pid, sizeof(pid_t)) == NULL)
		return E_MEM;
	else
		return E_SUCC;
}

static int
sys_getppid(uintptr_t ppid_la)
{
	if (ppid_la < VM_USERLO || ppid_la + sizeof(pid_t) > VM_USERHI)
		return E_INVAL_ADDR;

	struct proc *p = proc_cur();

	if (copy_to_user(p->pmap, ppid_la,
			 (uintptr_t) &p->parent->pid, sizeof(pid_t)) == NULL)
		return E_MEM;
	else
		return E_SUCC;
}

static int
sys_getchid(uintptr_t chid_la)
{
	struct proc *cur_p;
	struct channel *ch;
	int chid;

	if (!(VM_USERLO <= chid_la && chid_la + sizeof(int) <= VM_USERHI))
		return E_INVAL_ADDR;

	if ((cur_p = proc_cur()) == NULL)
		return E_INVAL_PID;

	if ((ch = cur_p->parent_ch) == NULL)
		return E_INVAL_CHID;

	chid = channel_getid(ch);

	if (copy_to_user(cur_p->pmap, chid_la,
			 (uintptr_t) &chid, sizeof(int)) == NULL)
		return E_MEM;

	return E_SUCC;
}

static int
sys_session(int type, uintptr_t sid_la)
{
	if (sid_la < VM_USERLO || sid_la + sizeof(sid_t) > VM_USERHI)
		return E_INVAL_ADDR;

	struct proc *p = proc_cur();
	struct session *s, *old_s;

	if ((s = session_new(type)) == NULL)
		return E_INVAL_SID;

	old_s = p->session;

	sched_lock(p->cpu);

	if (session_remove_proc(old_s, p)) {
		session_free(s);
		sched_unlock(p->cpu);
		return E_INVAL_SID;
	}

	if (session_add_proc(s, p)) {
		session_free(s);
		session_add_proc(old_s, p);
		sched_unlock(p->cpu);
		return E_INVAL_SID;
	}

	sched_unlock(p->cpu);

	if (copy_to_user(p->pmap, sid_la,
			 (uintptr_t) &s->sid, sizeof(int)) == NULL)
		return E_MEM;

	return E_SUCC;

}

static int
sys_getsid(uintptr_t sid_la)
{
	if (sid_la < VM_USERLO || sid_la + sizeof(sid_t) > VM_USERHI)
		return E_INVAL_ADDR;

	struct proc *p = proc_cur();

	if (copy_to_user(p->pmap, sid_la,
			 (uintptr_t) &p->session->sid, sizeof(sid_t)) == NULL)
		return E_MEM;
	else
		return E_SUCC;
}

static int
sys_send(int chid, uintptr_t msg_la, size_t size)
{
	struct proc *sender, *receiver;
	struct channel *ch;

	if (size > CHANNEL_BUFFER_SIZE)
		return E_SEND;

	if (!(VM_USERLO <= msg_la && msg_la + size <= VM_USERHI))
		return E_INVAL_ADDR;

	if ((ch = channel_getch(chid)) == NULL)
		return E_INVAL_CHID;

	if ((sender = proc_cur()) == NULL ||
	    (sender != ch->p1 && sender != ch->p2))
		return E_INVAL_PID;

	receiver = (sender == ch->p1) ? ch->p2 : ch->p1;

	if (sender->session != receiver->session)
		return E_INVAL_SID;

	if (copy_from_user((uintptr_t) sender->sys_buf,
			   sender->pmap, msg_la, size) == NULL)
		return E_MEM;

	if (proc_send_msg(ch, sender, sender->sys_buf, size))
		return E_SEND;

	return E_SUCC;
}

static int
sys_recv(int chid, uintptr_t buf_la, uintptr_t size_la)
{
	struct proc *sender, *receiver;
	struct channel *ch;
	size_t size;

	if (!(VM_USERLO <= size_la && size_la + sizeof(size_t) <= VM_USERHI))
		return E_INVAL_ADDR;

	if ((ch = channel_getch(chid)) == NULL)
		return E_INVAL_CHID;

	if ((receiver = proc_cur()) == NULL ||
	    (receiver != ch->p1 && receiver != ch->p2))
		return E_INVAL_PID;

	sender = (receiver == ch->p1) ? ch->p2 : ch->p1;

	if (sender->session != receiver->session)
		return E_INVAL_SID;

	if (proc_recv_msg(ch, receiver, receiver->sys_buf, &size, TRUE))
		return E_RECV;

	if (!(VM_USERLO <= size_la && size_la + size <= VM_USERHI))
		return E_INVAL_ADDR;

	if (copy_to_user(receiver->pmap, buf_la,
			 (uintptr_t) receiver->sys_buf, size) == NULL)
		return E_MEM;

	if (copy_to_user(receiver->pmap, size_la,
			 (uintptr_t) &size, sizeof(size_t)) == NULL)
		return E_MEM;

	return E_SUCC;
}

static int
sys_new_vm(uintptr_t vmid_la)
{
	struct proc *p = proc_cur();

	if (p->vm != NULL)
		return E_INVAL_PID;

	struct session *s = p->session;

	if (s->type != SESSION_VM)
		return E_INVAL_SID;

	if ((p->vm = vmm_init_vm()) == NULL) {
		proc_unlock(p);
		return E_INVAL_VMID;
	}

	if (copy_to_user(p->pmap, vmid_la,
			 (uintptr_t) &p->vm->vmid, sizeof(vmid_t)) == NULL)
		return E_MEM;

	p->vm->proc = p;
	s->vm = p->vm;

	return E_SUCC;
}

static int
sys_run_vm(void)
{
	struct proc *p = proc_cur();
	struct pcpu *c = p->cpu;

	if (c->vm != NULL || p->vm == NULL)
		return E_INVAL_VMID;

	vmm_run_vm(p->vm);

	return E_SUCC;
}

static int
sys_attach_vdev(pid_t pid, uintptr_t vid_la)
{
	struct proc *vm_p, *dev_p;
	vid_t vid;

	if (!(VM_USERLO <= vid_la && vid_la + sizeof(vid_t) <= VM_USERHI))
		return E_INVAL_ADDR;

	if ((vm_p = proc_cur()) == NULL)
		return E_INVAL_VMID;

	if ((dev_p = proc_pid2proc(pid)) == NULL)
		return E_INVAL_PID;

	if (dev_p->session != vm_p->session)
		return E_INVAL_SID;

	if ((vid = vdev_register_device(vm_p->vm, dev_p)) == -1)
		return E_ATTACH;

	if (copy_to_user(vm_p->pmap, vid_la,
			 (uintptr_t) &vid, sizeof(vid_t)) == NULL)
		return E_MEM;

	return E_SUCC;
}

static int
sys_detach_vdev(vid_t vid)
{
	struct proc *p = proc_cur();

	if (p->vm == NULL)
		return E_INVAL_VMID;

	if (vdev_get_dev(p->vm, vid) == NULL)
		return E_INVAL_VID;

	vdev_unregister_device(p->vm, vid);
	return E_SUCC;
}

static int
sys_setup_port(uint16_t port, data_sz_t width, int attach)
{
	struct proc *dev_p;
	struct session *session;
	vid_t vid;
	struct vm *vm;

	dev_p = proc_cur();
	session = dev_p->session;
	vm = session->vm;
	vid = dev_p->vid;

	if (vm == NULL)
		return E_INVAL_SID;

	if (vid == -1)
		return E_INVAL_VID;

	if (attach) {
		if (vdev_register_ioport(vm, port, width, vid))
			return E_ATTACH;
	} else {
		if (vdev_unregister_ioport(vm, port, width, vid))
			return E_DETACH;
	}

	return E_SUCC;
}

#define sys_attach_port(port, width)	sys_setup_port((port), (width), 1)
#define sys_detach_port(port, width)	sys_setup_port((port), (width), 0)

static int
sys_setup_irq(uint8_t irq, int attach)
{
	struct proc *dev_p;
	struct session *session;
	vid_t vid;
	struct vm *vm;

	dev_p = proc_cur();
	session = dev_p->session;
	vm = session->vm;
	vid = dev_p->vid;

	if (vm == NULL)
		return E_INVAL_SID;

	if (vid == -1)
		return E_INVAL_VID;

	if (!(0 <= irq && irq < MAX_IRQ))
		return E_INVAL_IRQ;

	if (attach) {
		if (vdev_register_irq(vm, irq, vid))
			return E_ATTACH;
	} else {
		if (vdev_unregister_irq(vm, irq, vid))
			return E_DETACH;
	}

	return E_SUCC;
}

#define sys_attach_irq(irq)	sys_setup_irq((irq), 1)
#define sys_detach_irq(irq)	sys_setup_irq((irq), 0)

static int
sys_ioport_inout(uintptr_t user_ioport_la, uintptr_t val_la, int in)
{
	struct proc *dev_p = proc_cur();
	struct vm *vm;
	int rc;

	struct user_ioport portinfo;
	uint32_t val;

	if ((vm = dev_p->session->vm) == NULL)
		return E_INVAL_VMID;

	if (dev_p->session != vm->proc->session)
		return E_INVAL_SID;

	if (dev_p->vid == -1)
		return E_INVAL_VID;

	if ((copy_from_user((uintptr_t) &portinfo, dev_p->pmap, user_ioport_la,
			    sizeof(struct user_ioport))) == NULL)
		return E_MEM;

	if (in) {
		rc = vdev_read_host_ioport(vm, dev_p->vid, portinfo.port,
					   portinfo.width, &val);

		if (rc)
			return E_IOPORT;

		if (copy_to_user(dev_p->pmap, val_la,
				 (uintptr_t) &val, 1 << portinfo.width) == NULL)
			return E_MEM;
	} else {
		val = val_la;

		rc = vdev_write_host_ioport(vm, dev_p->vid, portinfo.port,
					    portinfo.width, val);

		if (rc)
			return E_IOPORT;
	}

	return E_SUCC;
}

#define sys_host_in(port, val)		sys_ioport_inout((port), (val), 1)
#define sys_host_out(port, val)		sys_ioport_inout((port), (val), 0)

static int
sys_set_irq(uint8_t irq, int mode)
{
	struct proc *dev_p = proc_cur();
	struct vm *vm = dev_p->session->vm;

	if (vm == NULL)
		return E_INVAL_VMID;

	if (dev_p->session != vm->proc->session)
		return E_INVAL_SID;

	if (dev_p->vid == -1)
		return E_INVAL_VID;

	if (!(0 <= irq && irq < MAX_IRQ))
		return E_INVAL_IRQ;

	if (!(0 <= mode && mode < 3))
		return E_INVAL_MODE;

	if (vdev_set_irq(vm, dev_p->vid, irq, mode))
		return E_PIC;

	return E_SUCC;
}

static int
sys_guest_rw(uintptr_t gpa, uintptr_t la, size_t size, int write)
{
	struct proc *dev_p = proc_cur();
	struct vm *vm = dev_p->session->vm;

	if (la < VM_USERLO || la > VM_USERHI - size ||
	    VM_PHY_MEMORY_SIZE - la < size)
		return E_INVAL_ADDR;

	if (vm == NULL)
		return E_INVAL_VMID;

	if (dev_p->session != vm->proc->session)
		return E_INVAL_SID;

	if (dev_p->vid == -1)
		return E_INVAL_VID;

	if (vdev_rw_guest_mem(vm, gpa, dev_p->pmap, la, size, write))
		return E_MEM;

	return E_SUCC;
}

#define sys_guest_read(gpa, la, size)	sys_guest_rw((gpa), (la), (size), 0)
#define sys_guest_write(gpa, la, size)	sys_guest_rw((gpa), (la), (size), 1)

static int
sys_guest_tsc(uintptr_t tsc_la)
{
	struct proc *dev_p = proc_cur();
	struct vm *vm = dev_p->session->vm;

	uint64_t guest_tsc;

	if (tsc_la < VM_USERLO || VM_USERHI - tsc_la < sizeof(uint64_t))
		return E_INVAL_ADDR;

	if (vm == NULL)
		return E_INVAL_VMID;

	if (dev_p->session != vm->proc->session)
		return E_INVAL_SID;

	if (dev_p->vid == -1)
		return E_INVAL_VID;

	guest_tsc = vmm_rdtsc(vm);

	if (copy_to_user(dev_p->pmap, tsc_la,
			 (uintptr_t) &guest_tsc, sizeof(uint64_t)) == NULL)
		return E_MEM;

	return E_SUCC;
}

static int
sys_guest_cpufreq(uintptr_t freq_la)
{
	struct proc *dev_p = proc_cur();
	struct vm *vm = dev_p->session->vm;

	uint64_t freq;

	if (freq_la < VM_USERLO || VM_USERHI - freq_la < sizeof(uint64_t))
		return E_INVAL_ADDR;

	if (vm == NULL)
		return E_INVAL_VMID;

	if (dev_p->session != vm->proc->session)
		return E_INVAL_SID;

	if (dev_p->vid == -1)
		return E_INVAL_VID;

	freq = VM_TSC_FREQ;

	if (copy_to_user(dev_p->pmap, freq_la,
			 (uintptr_t) &freq, sizeof(uint64_t)) == NULL)
		return E_MEM;

	return E_SUCC;
}

static int
sys_guest_memsize(uintptr_t size_la)
{
	struct proc *dev_p = proc_cur();
	struct vm *vm = dev_p->session->vm;

	uint64_t size;

	if (size_la < VM_USERLO || VM_USERHI - size_la < sizeof(uint64_t))
		return E_INVAL_ADDR;

	if (vm == NULL)
		return E_INVAL_VMID;

	if (dev_p->session != vm->proc->session)
		return E_INVAL_SID;

	if (dev_p->vid == -1)
		return E_INVAL_VID;

	size = VM_PHY_MEMORY_SIZE;

	if (copy_to_user(dev_p->pmap, size_la,
			 (uintptr_t) &size, sizeof(uint64_t)) == NULL)
		return E_MEM;

	return E_SUCC;
}

static int
__sys_disk_read(uint64_t lba, uint64_t nsectors, uintptr_t buf)
{
	uint64_t cur_lba, remaining;
	uintptr_t cur_la;

	struct proc *p = proc_cur();

	cur_lba = lba;
	remaining = nsectors;
	cur_la = buf;

	while (remaining > 0) {
		uint64_t n = MIN(remaining, PAGESIZE / ATA_SECTOR_SIZE);
		if (ahci_disk_read(0, cur_lba, n, p->sys_buf))
			return E_DISK_OP;
		if (copy_to_user(p->pmap, cur_la, (uintptr_t) p->sys_buf,
				 n * ATA_SECTOR_SIZE) == NULL)
			return E_MEM;
		cur_lba += n;
		remaining -= n;
		cur_la += n * ATA_SECTOR_SIZE;
	}

	return E_SUCC;
}

static int
__sys_disk_write(uint64_t lba, uint64_t nsectors, uintptr_t buf)
{
	uint64_t cur_lba, remaining;
	uintptr_t cur_la;

	struct proc *p = proc_cur();

	cur_lba = lba;
	remaining = nsectors;
	cur_la = buf;

	while (remaining > 0) {
		uint64_t n = MIN(remaining, PAGESIZE / ATA_SECTOR_SIZE);
		if (copy_from_user((uintptr_t) p->sys_buf, p->pmap, cur_la,
				   n * ATA_SECTOR_SIZE) == NULL)
			return E_MEM;
		if (ahci_disk_write(0, cur_lba, n, p->sys_buf))
			return E_DISK_OP;
		cur_lba += n;
		remaining -= n;
		cur_la += n * ATA_SECTOR_SIZE;
	}

	return E_SUCC;
}

static int
sys_disk_op(uintptr_t dop_la)
{
	struct proc *p;
	pmap_t *pmap;

	struct user_disk_op dop;
	int rc = 0;

	if (dop_la < VM_USERLO || dop_la + sizeof(dop) > VM_USERHI)
		return E_INVAL_ADDR;

	p = proc_cur();
	pmap = p->pmap;

	if (copy_from_user((uintptr_t) &dop, pmap, dop_la, sizeof(dop)) == NULL)
		return E_MEM;

	if (!(VM_USERLO <= dop.buf &&
	      dop.buf + dop.n * ATA_SECTOR_SIZE <= VM_USERHI))
		return E_INVAL_ADDR;

	switch (dop.type) {
	case DISK_READ:
		rc = __sys_disk_read(dop.lba, dop.n, dop.buf);
		break;
	case DISK_WRITE:
		rc = __sys_disk_write(dop.lba, dop.n, dop.buf);
		break;
	default:
		rc = 1;
	}

	if (rc)
		return E_DISK_OP;

	return E_SUCC;
}

static int
sys_disk_cap(uintptr_t lo_la, uintptr_t hi_la)
{
	uint64_t cap;
	uint32_t cap_lo, cap_hi;

	if (!(VM_USERLO <= lo_la && lo_la + sizeof(uint32_t) <= VM_USERHI))
		return E_INVAL_ADDR;
	if (!(VM_USERLO <= hi_la && hi_la + sizeof(uint32_t) <= VM_USERHI))
		return E_INVAL_ADDR;

	cap = ahci_disk_capacity(0);
	cap_lo = cap & 0xffffffff;
	cap_hi = (cap >> 32) & 0xffffffff;

	if ((copy_to_user(proc_cur()->pmap, lo_la,
			  (uintptr_t) &cap_lo, sizeof(uint32_t))) == NULL)
		return E_MEM;
	if ((copy_to_user(proc_cur()->pmap, hi_la,
			  (uintptr_t) &cap_hi, sizeof(uint32_t))) == NULL)
		return E_MEM;

	return E_SUCC;
}

/*
 * Syetem calls in CertiKOS follow the convention below.
 *
 * - The system call number is passed through the first argument of the user
 *   context, e.g. %eax on i386 arch.
 *
 * - Each system call can have at most three parameters, which are passed
 *   through the second argument to the fourth argument of the user context
 *   respectively, e.g. %ebx, %ecx, and %edx on i386 arch.
 *
 * - The error code of the syscall is returned through the first argument of
 *   the user context, i.e. %eax on i386 arch. Error code 0 means no errors.
 *
 * - Changes to other arguments maybe not returned to the caller.
 */
int
syscall_handler(struct context *ctx, int guest)
{
	KERN_ASSERT(ctx != NULL);
	KERN_ASSERT(!guest);

	uint32_t nr = ctx_arg1(ctx);
	uint32_t a[3] =
		{ctx_arg2(ctx), ctx_arg3(ctx), ctx_arg4(ctx)};
	uint32_t errno = E_SUCC;

	NR_DEBUG(nr, "from 0x%08x.\n", ctx->tf.eip);
	/* ctx_dump(ctx); */

	memzero(proc_cur()->sys_buf, PAGESIZE);

	switch (nr) {
	case SYS_puts:
		/*
		 * Output a string to the screen.
		 * a[0]: the linear address where the string is
		 */
		errno = sys_puts((uintptr_t) a[0]);
		break;
	case SYS_getc:
		/*
		 * Input a character from the keyboard.
		 * a[0]: the linear address where the input will be returned to
		 */
		errno = sys_getc((uintptr_t) a[0]);
		break;
	case SYS_create_proc:
		/*
		 * Create a new process.
		 * a[0]: the linear address where the binary code is
		 * a[1]: the linear address where hte process ID is returned to
		 */
		errno = sys_create_proc((uintptr_t) vdev_elf_addr[(int) a[0]],
					(uintptr_t) a[1]);
		break;
	case SYS_run_proc:
		/*
		 * Run a process.
		 * a[0]: the process ID
		 * a[1]: the CPU index on which the process is going to run
		 */
		errno = sys_run_proc((uint32_t) a[0], (pid_t) a[1]);
		break;
	case SYS_yield:
		/*
		 * Called by a process to abandon its CPU slice.
		 */
		errno = sys_yield();
		break;
	case SYS_getpid:
		/*
		 * Get the process id of the caller.
		 * a[0]: the linear address where the process id will be
		 *       returned to
		 */
		errno = sys_getpid((uintptr_t) a[0]);
		break;
	case SYS_getppid:
		/*
		 * Get the process id of the parent process of the caller.
		 * a[0]: the linear address where the process id will be
		 *       returned to
		 */
		errno = sys_getppid((uintptr_t) a[0]);
		break;
	case SYS_getchid:
		/*
		 * Get the channel ID to the parent process.
		 * a[0]: the linear address where the channel ID is returned to
		 */
		errno = sys_getchid((uintptr_t) a[0]);
		break;
	case SYS_session:
		/*
		 * Create a new session. The caller will be moved from its
		 * current session to the new session after a successful
		 * creation.
		 * a[0]: the type of the session:
		 *       0 indicates a normal session,
		 *       1 indicates a session for a virtual machine
		 * a[1]: the linear address where the session id of the new
		 *       session will be returned to
		 */
		errno = sys_session((int) a[0], (uintptr_t) a[1]);
		break;
	case SYS_getsid:
		/*
		 * Get the session id of the caller.
		 * a[0]: the linear address where the session id will be
		 *       returned to
		 */
		errno = sys_getsid((uintptr_t) a[0]);
	case SYS_send:
		/*
		 * Send a message to a channel.
		 * a[0]: the identity of the channel
		 * a[1]: the linear address where the message is stored
		 * a[2]: the size of the message
		 */
		errno = sys_send((int) a[0], (uintptr_t) a[1], (size_t) a[2]);
		break;
	case SYS_recv:
		/*
		 * Receive a message from a channel.
		 * a[0]: the identity of the channel
		 * a[1]: the linear address where the message will be stored
		 * a[2]: the linear address where the size of the message will
		 *       be stored
		 */
		errno = sys_recv((int) a[0], (uintptr_t) a[1], (uintptr_t) a[2]);
		break;
	case SYS_new_vm:
		/*
		 * Create a new virtual machine in the session of the caller.
		 * a[0]: the linear address where the virtual machine id will be
		 *       returned to
		 */
		errno = sys_new_vm((uintptr_t) a[0]);
		break;
	case SYS_run_vm:
		/*
		 * Run the virtual machine in the session of the caller.
		 */
		errno = sys_run_vm();
		break;
	case SYS_attach_vdev:
		/*
		 * Attach a process as a virtual device to the virtual machine
		 * in the session of the caller. A virtual device id will be
		 * be allocated for the virtual device.
		 * a[0]: the process ID
		 * a[1]: the linear address where the virtual device ID will be
		 *       returned to
		 */
		errno = sys_attach_vdev((pid_t) a[0], (uintptr_t) a[1]);
		break;
	case SYS_detach_vdev:
		/*
		 * Detach a virtual device from the virtual machine in the
		 * session of the caller.
		 * a[0]: the virtual device ID
		 */
		errno = sys_detach_vdev((vid_t) a[0]);
		break;
	case SYS_attach_port:
		/*
		 * Attach an I/O port to a virtual device of the virtual machine
		 * in the session of the caller.
		 * a[0]: the I/O port
		 * a[1]: the data width
		 */
		errno = sys_attach_port((uint16_t) a[0], (data_sz_t) a[1]);
		break;
	case SYS_detach_port:
		/*
		 * Detach an I/O port from a virtual device of the virtual
		 * machine in the session of the caller.
		 * a[0]: the I/O port
		 * a[1]: the data width
		 */
		errno = sys_detach_port((uint16_t) a[0], (data_sz_t) a[1]);
		break;
	case SYS_attach_irq:
		/*
		 * Assign an IRQ to a virtual device.
		 * a[0]: the IRQ number
		 */
		errno = sys_attach_irq((uint8_t) a[0]);
		break;
	case SYS_detach_irq:
		/*
		 * Remove an IRQ from a virtual device.
		 * a[0]: the IRQ number
		 */
		errno = sys_detach_irq((uint8_t) a[0]);
		break;
	case SYS_host_in:
		/*
		 * Read a host I/O port. The information of the I/O port
		 * must be provided in an object of type struct user_ioport
		 * by the caller.
		 * a[0]: the linear address where the user_ioport object is
		 * a[1]: the linear address where the read value will be
		 *       returned to
		 */
		errno = sys_host_in((uintptr_t) a[0], (uintptr_t) a[1]);
		break;
	case SYS_host_out:
		/*
		 * Write a host I/O port. The information of the I/O port
		 * must be provided in an object of type struct user_ioport by
		 * the caller.
		 * a[0]: the linear address where the user_ioport object is
		 * a[1]: the value which will be written to the I/O port
		 */
		errno = sys_host_out((uintptr_t) a[0], (uint32_t) a[1]);
		break;
	case SYS_set_irq:
		/*
		 * Set the IRQ line of the virtual PIC.
		 * a[0]: the IRQ number
		 * a[1]: 0 indicates raising the IRQ line,
		 *       1 indicates lowering the IRQ line,
		 *       2 indicates trigger the IRQ line
		 */
		errno = sys_set_irq((uint8_t) a[0], (int) a[1]);
		break;
	case SYS_guest_read:
		/*
		 * Transfer data from the guest physical address to the host
		 * linear address.
		 * a[0]: the guest physical address
		 * a[1]: the host linear address
		 * a[2]: how many bytes will be transferred
		 */
		errno = sys_guest_read
			((uintptr_t) a[0], (uintptr_t) a[1], (size_t) a[2]);
		break;
	case SYS_guest_write:
		/*
		 * Transfer data from the host linear address to the guest
		 * physical address.
		 * a[0]: the guest physical address
		 * a[1]: the host linear address
		 * a[2]: how many bytes will be transferred
		 */
		errno = sys_guest_write
			((uintptr_t) a[0], (uintptr_t) a[1], (size_t) a[2]);
		break;
	case SYS_guest_tsc:
		/*
		 * Read the guest TSC.
		 * a[0]: the linear address where the valuse of guest TSC will
		 *       be returned to
		 */
		errno = sys_guest_tsc((uintptr_t) a[0]);
		break;
	case SYS_guest_cpufreq:
		/*
		 * Get the frequency of guest TSC.
		 * a[0]: the linear address where the frequency will be returned
		 *       to
		 */
		errno = sys_guest_cpufreq((uintptr_t) a[0]);
		break;
	case SYS_guest_memsize:
		/*
		 * Get the size of the guest physical memory.
		 * a[0]: the linear address where the size will be returned to
		 */
		errno = sys_guest_memsize((uintptr_t) a[0]);
		break;
	case SYS_disk_op:
		/*
		 * Disk operation. The operation information must be provided in
		 * an object of type struct user_disk_op by the caller.
		 * a[0]: the linear address where the user_disk_op is
		 */
		errno = sys_disk_op((uintptr_t) a[0]);
		break;
	case SYS_disk_cap:
		/*
		 * Get the capability of the disk for the virtual machine.
		 * a[0]: the linear address where the lowest 32 bits of the
		 *       capability are returned to
		 * a[1]: the linear address where the highest 32 bits of the
		 *       capability are returned to
		 */
		errno = sys_disk_cap((uintptr_t) a[0], (uintptr_t) a[1]);
		break;

	default:
		errno = E_INVAL_CALLNR;
		break;
	}

	ctx_set_retval(ctx, errno);

	if (errno)
		NR_DEBUG(nr, "failed (error %s).\n", ERRNO_STR(errno));
	else
		NR_DEBUG(nr, "done.\n");
	return errno;
}