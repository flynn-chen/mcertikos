#ifndef _SYS_DEV_TSC_H_
#define _SYS_DEV_TSC_H_

#ifdef _KERN_

volatile uint64_t tsc_per_ms;	/* TSC ticks per microsecond */

int tsc_init(void);

#endif /* _KERN_ */

#endif /* !_SYS_DEV_TSC_H_ */
