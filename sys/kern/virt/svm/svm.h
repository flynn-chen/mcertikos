#ifndef _VIRT_SVM_H_
#define _VIRT_SVM_H_

#ifdef _KERN_

#include <sys/gcc.h>
#include <sys/types.h>
#include <sys/virt/hvm.h>

enum {
	INTERCEPT_INTR,
	INTERCEPT_NMI,
	INTERCEPT_SMI,
	INTERCEPT_INIT,
	INTERCEPT_VINTR,
	INTERCEPT_SELECTIVE_CR0,
	INTERCEPT_STORE_IDTR,
	INTERCEPT_STORE_GDTR,
	INTERCEPT_STORE_LDTR,
	INTERCEPT_STORE_TR,
	INTERCEPT_LOAD_IDTR,
	INTERCEPT_LOAD_GDTR,
	INTERCEPT_LOAD_LDTR,
	INTERCEPT_LOAD_TR,
	INTERCEPT_RDTSC,
	INTERCEPT_RDPMC,
	INTERCEPT_PUSHF,
	INTERCEPT_POPF,
	INTERCEPT_CPUID,
	INTERCEPT_RSM,
	INTERCEPT_IRET,
	INTERCEPT_INTn,
	INTERCEPT_INVD,
	INTERCEPT_PAUSE,
	INTERCEPT_HLT,
	INTERCEPT_INVLPG,
	INTERCEPT_INVLPGA,
	INTERCEPT_IOIO_PROT,
	INTERCEPT_MSR_PROT,
	INTERCEPT_TASK_SWITCH,
	INTERCEPT_FERR_FREEZE,
	INTERCEPT_SHUTDOWN,
	INTERCEPT_VMRUN,
	INTERCEPT_VMMCALL,
	INTERCEPT_VMLOAD,
	INTERCEPT_VMSAVE,
	INTERCEPT_STGI,
	INTERCEPT_CLGI,
	INTERCEPT_SKINIT,
	INTERCEPT_RDTSCP,
	INTERCEPT_ICEBP,
	INTERCEPT_WBINVD,
	INTERCEPT_MONITOR,
	INTERCEPT_MWAIT,
	INTERCEPT_MWAIT_COND,
	INTERCEPT_XSETBV,
};

#ifdef __COMPCERT__
#pragma pack(1)
#endif
struct vmcb_control_area {
	uint32_t	intercept_cr;
	uint32_t	intercept_dr;
	uint32_t	intercept_exceptions;
#ifndef __COMPCERT__
	uint64_t	intercept;
#else
	uint32_t	intercept_lo;
	uint32_t	intercept_hi;
#endif
	uint8_t 	reserved_1[42];
	uint16_t 	pause_filter_count;
#ifndef __COMPCERT__
	uint64_t 	iopm_base_pa;
	uint64_t 	msrpm_base_pa;
	uint64_t 	tsc_offset;
#else
	uint32_t	iopm_base_pa_lo;
	uint32_t	iopm_base_pa_hi;
	uint32_t	msrpm_base_pa_lo;
	uint32_t	msrpm_base_pa_hi;
	uint32_t	tsc_offset_lo;
	uint32_t	tsc_offset_hi;
#endif
	uint32_t 	asid;
	uint8_t 	tlb_ctl;
	uint8_t 	reserved_2[3];
	uint32_t 	int_ctl;
	uint32_t 	int_vector;
	uint32_t 	int_state;
	uint8_t 	reserved_3[4];
	uint32_t 	exit_code;
	uint32_t 	exit_code_hi;
#ifndef __COMPCERT__
	uint64_t 	exit_info_1;
	uint64_t 	exit_info_2;
#else
	uint32_t	exit_info_1_lo;
	uint32_t	exit_info_1_hi;
	uint32_t	exit_info_2_lo;
	uint32_t	exit_info_2_hi;
#endif
	uint32_t 	exit_int_info;
	uint32_t 	exit_int_info_err;
#ifndef __COMPCERT__
	uint64_t 	nested_ctl;
#else
	uint32_t	nested_ctl_lo;
	uint32_t	nested_ctl_hi;
#endif
	uint8_t 	reserved_4[16];
	uint32_t 	event_inj;
	uint32_t 	event_inj_err;
#ifndef __COMPCERT__
	uint64_t 	nested_cr3;
	uint64_t 	lbr_ctl;
#else
	uint32_t	nested_cr3_lo;
	uint32_t	nested_cr3_hi;
	uint32_t	lbr_ctl_lo;
	uint32_t	lbr_ctl_hi;
#endif
	uint32_t 	clean;
	uint32_t 	reserved_5;
#ifndef __COMPCERT__
	uint64_t 	next_rip;
#else
	uint32_t	next_rip_lo;
	uint32_t	next_rip_hi;
#endif
	uint8_t 	insn_len;
	uint8_t 	insn_bytes[15];
	uint8_t 	reserved_6[800];
} gcc_packed;

struct vmcb_seg {
	uint16_t 	selector;
	uint16_t 	attrib;
	uint32_t 	limit;
#ifndef __COMPCERT__
	uint64_t 	base;
#else
	uint32_t	base_lo;
	uint32_t	base_hi;
#endif
};

#ifdef __COMPCERT__
#pragma pack(1)
#endif
struct vmcb_save_area {
	struct vmcb_seg	es;
	struct vmcb_seg	cs;
	struct vmcb_seg	ss;
	struct vmcb_seg	ds;
	struct vmcb_seg	fs;
	struct vmcb_seg	gs;
	struct vmcb_seg	gdtr;
	struct vmcb_seg ldtr;
	struct vmcb_seg idtr;
	struct vmcb_seg tr;
	uint8_t 	reserved_1[43];
	uint8_t 	cpl;
	uint8_t 	reserved_2[4];
#ifndef __COMPCERT__
	uint64_t 	efer;
#else
	uint32_t	efer_lo;
	uint32_t	efer_hi;
#endif
	uint8_t 	reserved_3[112];
#ifndef __COMPCERT__
	uint64_t	cr4;
	uint64_t 	cr3;
	uint64_t 	cr0;
	uint64_t 	dr7;
	uint64_t 	dr6;
	uint64_t 	rflags;
	uint64_t 	rip;
#else
	uint32_t	cr4_lo;
	uint32_t	cr4_hi;
	uint32_t	cr3_lo;
	uint32_t	cr3_hi;
	uint32_t	cr0_lo;
	uint32_t	cr0_hi;
	uint32_t	dr7_lo;
	uint32_t	dr7_hi;
	uint32_t	dr6_lo;
	uint32_t	dr6_hi;
	uint32_t	rflags_lo;
	uint32_t	rflags_hi;
	uint32_t	rip_lo;
	uint32_t	rip_hi;
#endif
	uint8_t 	reserved_4[88];
#ifndef __COMPCERT__
	uint64_t 	rsp;
#else
	uint32_t	rsp_lo;
	uint32_t	rsp_hi;
#endif
	uint8_t 	reserved_5[24];
#ifndef __COMPCERT__
	uint64_t 	rax;
	uint64_t 	star;
	uint64_t 	lstar;
	uint64_t 	cstar;
	uint64_t 	sfmask;
	uint64_t 	kernel_gs_base;
	uint64_t 	sysenter_cs;
	uint64_t 	sysenter_esp;
	uint64_t 	sysenter_eip;
	uint64_t 	cr2;
#else
	uint32_t 	rax_lo;
	uint32_t	rax_hi;
	uint32_t	star_lo;
	uint32_t 	star_hi;
	uint32_t 	lstar_lo;
	uint32_t 	lstar_hi;
	uint32_t 	cstar_lo;
	uint32_t 	cstar_hi;
	uint32_t 	sfmask_lo;
	uint32_t 	sfmask_hi;
	uint32_t 	kernel_gs_base_lo;
	uint32_t 	kernel_gs_base_hi;
	uint32_t 	sysenter_cs_lo;
	uint32_t 	sysenter_cs_hi;
	uint32_t 	sysenter_esp_lo;
	uint32_t 	sysenter_esp_hi;
	uint32_t 	sysenter_eip_lo;
	uint32_t 	sysenter_eip_hi;
	uint32_t 	cr2_lo;
	uint32_t 	cr2_hi;
#endif
	uint8_t 	reserved_6[32];
#ifndef __COMPCERT__
	uint64_t 	g_pat;
	uint64_t 	dbgctl;
	uint64_t 	br_from;
	uint64_t 	br_to;
	uint64_t 	last_excp_from;
	uint64_t 	last_excp_to;
#else
	uint32_t 	g_pat_lo;
	uint32_t 	g_pat_hi;
	uint32_t 	dbgctl_lo;
	uint32_t 	dbgctl_hi;
	uint32_t 	br_from_lo;
	uint32_t 	br_from_hi;
	uint32_t 	br_to_lo;
	uint32_t 	br_to_hi;
	uint32_t 	last_excp_from_lo;
	uint32_t 	last_excp_from_hi;
	uint32_t 	last_excp_to_lo;
	uint32_t 	last_excp_to_hi;
#endif
} gcc_packed;

#ifdef __COMPCERT__
#pragma pack(1)
#endif
struct vmcb {
	struct vmcb_control_area control;
	struct vmcb_save_area	 save;
} gcc_packed;

#define INTERCEPT_CR0_READ	0
#define INTERCEPT_CR3_READ	3
#define INTERCEPT_CR4_READ	4
#define INTERCEPT_CR8_READ	8
#define INTERCEPT_CR0_WRITE	(16 + 0)
#define INTERCEPT_CR3_WRITE	(16 + 3)
#define INTERCEPT_CR4_WRITE	(16 + 4)
#define INTERCEPT_CR8_WRITE	(16 + 8)

#define INTERCEPT_DR0_READ	0
#define INTERCEPT_DR1_READ	1
#define INTERCEPT_DR2_READ	2
#define INTERCEPT_DR3_READ	3
#define INTERCEPT_DR4_READ	4
#define INTERCEPT_DR5_READ	5
#define INTERCEPT_DR6_READ	6
#define INTERCEPT_DR7_READ	7
#define INTERCEPT_DR0_WRITE	(16 + 0)
#define INTERCEPT_DR1_WRITE	(16 + 1)
#define INTERCEPT_DR2_WRITE	(16 + 2)
#define INTERCEPT_DR3_WRITE	(16 + 3)
#define INTERCEPT_DR4_WRITE	(16 + 4)
#define INTERCEPT_DR5_WRITE	(16 + 5)
#define INTERCEPT_DR6_WRITE	(16 + 6)
#define INTERCEPT_DR7_WRITE	(16 + 7)

#define SVM_EVTINJ_VEC_MASK 0xff

#define SVM_EVTINJ_TYPE_SHIFT 8
#define SVM_EVTINJ_TYPE_MASK (7 << SVM_EVTINJ_TYPE_SHIFT)

#define SVM_EVTINJ_TYPE_INTR (0 << SVM_EVTINJ_TYPE_SHIFT)
#define SVM_EVTINJ_TYPE_NMI (2 << SVM_EVTINJ_TYPE_SHIFT)
#define SVM_EVTINJ_TYPE_EXEPT (3 << SVM_EVTINJ_TYPE_SHIFT)
#define SVM_EVTINJ_TYPE_SOFT (4 << SVM_EVTINJ_TYPE_SHIFT)

#define SVM_EVTINJ_VALID (1 << 31)
#define SVM_EVTINJ_VALID_ERR (1 << 11)

#define SVM_EXITINTINFO_VEC_MASK SVM_EVTINJ_VEC_MASK
#define SVM_EXITINTINFO_TYPE_MASK SVM_EVTINJ_TYPE_MASK

#define	SVM_EXITINTINFO_TYPE_INTR SVM_EVTINJ_TYPE_INTR
#define	SVM_EXITINTINFO_TYPE_NMI SVM_EVTINJ_TYPE_NMI
#define	SVM_EXITINTINFO_TYPE_EXEPT SVM_EVTINJ_TYPE_EXEPT
#define	SVM_EXITINTINFO_TYPE_SOFT SVM_EVTINJ_TYPE_SOFT

#define SVM_EXITINTINFO_VALID SVM_EVTINJ_VALID
#define SVM_EXITINTINFO_VALID_ERR SVM_EVTINJ_VALID_ERR

#define SVM_EXITINFOSHIFT_TS_REASON_IRET 36
#define SVM_EXITINFOSHIFT_TS_REASON_JMP 38
#define SVM_EXITINFOSHIFT_TS_HAS_ERROR_CODE 44

#define SVM_EXITINFO_REG_MASK 0x0F

#define	SVM_EXIT_READ_CR0 	0x000
#define	SVM_EXIT_READ_CR3 	0x003
#define	SVM_EXIT_READ_CR4 	0x004
#define	SVM_EXIT_READ_CR8 	0x008
#define	SVM_EXIT_WRITE_CR0 	0x010
#define	SVM_EXIT_WRITE_CR3 	0x013
#define	SVM_EXIT_WRITE_CR4 	0x014
#define	SVM_EXIT_WRITE_CR8 	0x018
#define	SVM_EXIT_READ_DR0 	0x020
#define	SVM_EXIT_READ_DR1 	0x021
#define	SVM_EXIT_READ_DR2 	0x022
#define	SVM_EXIT_READ_DR3 	0x023
#define	SVM_EXIT_READ_DR4 	0x024
#define	SVM_EXIT_READ_DR5 	0x025
#define	SVM_EXIT_READ_DR6 	0x026
#define	SVM_EXIT_READ_DR7 	0x027
#define	SVM_EXIT_WRITE_DR0 	0x030
#define	SVM_EXIT_WRITE_DR1 	0x031
#define	SVM_EXIT_WRITE_DR2 	0x032
#define	SVM_EXIT_WRITE_DR3 	0x033
#define	SVM_EXIT_WRITE_DR4 	0x034
#define	SVM_EXIT_WRITE_DR5 	0x035
#define	SVM_EXIT_WRITE_DR6 	0x036
#define	SVM_EXIT_WRITE_DR7 	0x037
#define SVM_EXIT_EXCP_BASE      0x040
#define SVM_EXIT_INTR		0x060
#define SVM_EXIT_NMI		0x061
#define SVM_EXIT_SMI		0x062
#define SVM_EXIT_INIT		0x063
#define SVM_EXIT_VINTR		0x064
#define SVM_EXIT_CR0_SEL_WRITE	0x065
#define SVM_EXIT_IDTR_READ	0x066
#define SVM_EXIT_GDTR_READ	0x067
#define SVM_EXIT_LDTR_READ	0x068
#define SVM_EXIT_TR_READ	0x069
#define SVM_EXIT_IDTR_WRITE	0x06a
#define SVM_EXIT_GDTR_WRITE	0x06b
#define SVM_EXIT_LDTR_WRITE	0x06c
#define SVM_EXIT_TR_WRITE	0x06d
#define SVM_EXIT_RDTSC		0x06e
#define SVM_EXIT_RDPMC		0x06f
#define SVM_EXIT_PUSHF		0x070
#define SVM_EXIT_POPF		0x071
#define SVM_EXIT_CPUID		0x072
#define SVM_EXIT_RSM		0x073
#define SVM_EXIT_IRET		0x074
#define SVM_EXIT_SWINT		0x075
#define SVM_EXIT_INVD		0x076
#define SVM_EXIT_PAUSE		0x077
#define SVM_EXIT_HLT		0x078
#define SVM_EXIT_INVLPG		0x079
#define SVM_EXIT_INVLPGA	0x07a
#define SVM_EXIT_IOIO		0x07b
#define SVM_EXIT_MSR		0x07c
#define SVM_EXIT_TASK_SWITCH	0x07d
#define SVM_EXIT_FERR_FREEZE	0x07e
#define SVM_EXIT_SHUTDOWN	0x07f
#define SVM_EXIT_VMRUN		0x080
#define SVM_EXIT_VMMCALL	0x081
#define SVM_EXIT_VMLOAD		0x082
#define SVM_EXIT_VMSAVE		0x083
#define SVM_EXIT_STGI		0x084
#define SVM_EXIT_CLGI		0x085
#define SVM_EXIT_SKINIT		0x086
#define SVM_EXIT_RDTSCP		0x087
#define SVM_EXIT_ICEBP		0x088
#define SVM_EXIT_WBINVD		0x089
#define SVM_EXIT_MONITOR	0x08a
#define SVM_EXIT_MWAIT		0x08b
#define SVM_EXIT_MWAIT_COND	0x08c
#define SVM_EXIT_XSETBV		0x08d
#define SVM_EXIT_NPF  		0x400

#define SVM_EXIT_ERR		-1

#define CPUID_FEATURE_FUNC	0x80000001
# define CPUID_SVM_FEATURE_FUNC	0x8000000a
#define CPUID_FEATURE_SVM	(1<<2)
# define CPUID_SVM_LOCKED	(1<<2)

#define MSR_VM_CR		0xc0010114
# define MSR_VM_CR_SVMDIS	(1<<4)
# define MSR_VM_CR_LOCK		(1<<3)
# define MSR_VM_CR_DISA20	(1<<2)
# define MSR_VM_CR_RINIT	(1<<1)
# define MSR_VM_CR_DPD		(1<<0)

#define MSR_VM_HSAVE_PA		0xc0010117

#define SVM_IOPM_SIZE		(12 << 10)
#define SVM_MSRPM_SIZE		(8 << 10)

#define SEG_ATTR_G		(1 << 11)
#define SEG_ATTR_D		(1 << 10)
#define SEG_ATTR_B		(1 << 10)
#define SEG_ATTR_L		(1 << 9)
#define SEG_ATTR_AVL		(1 << 8)
#define SEG_ATTR_P		(1 << 7)
#define SEG_ATTR_DPL_SHIFT	5
#define SEG_ATTR_S		(1 << 4)

#define SEG_TYPE_CODE		0xa
#define SEG_TYPE_DATA		0x2
#define SEG_TYPE_LDT		0x2
#define SEG_TYPE_TSS_BUSY	0x3
#define SEG_TYPE_TSS		0xb

#define SVM_INTR_CTRL_VTPR		0xff
#define SVM_INTR_CTRL_VIRQ_SHIFT	8
#define SVM_INTR_CTRL_VIRQ		(1 << SVM_INTR_CTRL_VIRQ_SHIFT)
#define SVM_INTR_CTRL_PRIO_SHIFT	16
#define SVM_INTR_CTRL_PRIO		(0xf << SVM_INTR_CTRL_PRIO_SHIFT)
#define SVM_INTR_CTRL_IGN_VTPR_SHIFT	20
#define SVM_INTR_CTRL_IGN_VTPR		(1 << SVM_INTR_CTRL_IGN_VTPR_SHIFT)
#define SVM_INTR_CTRL_VINTR_MASK_SHIFT	24
#define SVM_INTR_CTRL_VINTR_MASK	(1 << SVM_INTR_CTRL_VINTR_MASK_SHIFT)
#define SVM_INTR_CTRL_VINTR_VEC_SHIFT	32
#define SVM_INTR_CTRL_VINTR_VEC		(0xff << SVM_INTR_CTRL_VINTR_VEC_SHIFT)

/* EXITINFO1 for IOIO interception */
#define SVM_EXITINFO1_PORT_SHIFT	16
#define SVM_EXITINFO1_PORT_MASK		(0xffff << SVM_EXITINFO1_PORT_SHIFT)
#define SVM_EXITINFO1_SEG_SHIFT		10
#define SVM_EXITINFO1_SEG_MASK		(0x7 << SVM_EXITINFO1_SEG_SHIFT)
#define SVM_EXITINFO1_A64		(1 << 9)
#define SVM_EXITINFO1_A32		(1 << 8)
#define SVM_EXITINFO1_A16		(1 << 7)
#define SVM_EXITINFO1_SZ32		(1 << 6)
#define SVM_EXITINFO1_SZ16		(1 << 5)
#define SVM_EXITINFO1_SZ8		(1 << 4)
#define SVM_EXITINFO1_REP		(1 << 3)
#define SVM_EXITINFO1_STR		(1 << 2)
#define SVM_EXITINFO1_TYPE_MASK		0x1
#define SVM_EXITINFO1_TYPE_OUT		0
#define SVM_EXITINFO1_TYPE_IN		1

/* EXITINFO1 for nested page fault */
#define SVM_EXITINFO1_NFP_P		(1ULL << 0)
#define SVM_EXITINFO1_NFP_W		(1ULL << 1)
#define SVM_EXITINFO1_NFP_U		(1ULL << 2)
#define SVM_EXITINFO1_NFP_RSV		(1ULL << 3)
#define SVM_EXITINFO1_NFP_ID		(1ULL << 4)
#define SVM_EXITINFO1_NFP_ADDR		(1ULL << 32)
#define SVM_EXITINFO1_NFP_WALK		(1ULL << 33)

#define SVM_VMRUN()					\
	do {						\
		__asm __volatile("vmrun");		\
	} while (0)

#define SVM_VMLOAD()					\
	do {						\
		__asm __volatile("vmload");		\
	} while (0)

#define SVM_VMSAVE()					\
	do {						\
		__asm __volatile("vmsave");		\
	} while (0)

#define SVM_STGI()					\
	do {						\
		__asm __volatile("stgi");		\
	} while (0)

#define SVM_CLGI()				\
	do {					\
		__asm __volatile("clgi");	\
	} while (0)

#ifdef __COMPCERT__
#pragma pack(1)
#endif
struct svm {
	/*
	 * VMCB does not store following registers for guest, so we have
	 * to do that by ourself.
	 */
	uint32_t	g_rbx, g_rcx, g_rdx, g_rsi, g_rdi, g_rbp;
	struct vmcb	*vmcb;		/* VMCB */
	int		inuse;
} gcc_packed;

#ifdef DEBUG_SVM

#define SVM_DEBUG(fmt, ...)				\
	do {						\
		KERN_DEBUG("SVM: "fmt, ##__VA_ARGS__);	\
	} while (0)

#else

#define SVM_DEBUG(fmt, ...)			\
	do {					\
	} while (0)

#endif

int svm_init(void);
int svm_init_vm(struct vm *vm);
int svm_run_vm(struct vm *vm);
int svm_intercept_vintr(struct vm *vm, bool enable);
int svm_get_reg(struct vm *vm, guest_reg_t reg, uint32_t *val);
int svm_set_reg(struct vm *vm, guest_reg_t reg, uint32_t val);
int svm_get_desc(struct vm *vm, guest_seg_t seg, struct guest_seg_desc *desc);
int svm_set_desc(struct vm *vm, guest_seg_t seg, struct guest_seg_desc *desc);
int svm_set_mmap(struct vm *vm, uintptr_t gpa, uintptr_t hpa, int type);
int svm_inject_event(struct vm *vm, guest_event_t type, uint8_t vector,
		     uint32_t errcode, bool ev);
int svm_get_next_eip(struct vm *vm, guest_instr_t instr, uint32_t *val);
bool svm_pending_event(struct vm *vm);
bool svm_intr_shadow(struct vm *vm);

#endif /* _KERN_ */

#endif /* !_VIRT_SVM_H_ */