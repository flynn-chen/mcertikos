#ifndef _KERN_THREAD_PTHREAD_H_
#define _KERN_THREAD_PTHREAD_H_

#ifdef _KERN_

void thread_init(unsigned int mbi_addr);
unsigned int thread_spawn(void *entry, unsigned int id,
                          unsigned int quota);
void thread_yield(void);
void sched_update(void);

unsigned int get_previous_id(void);
void set_previous_id(void);
void invalidate_previous_id(void);

#endif /* _KERN_ */

#endif /* !_KERN_THREAD_PTHREAD_H_ */
