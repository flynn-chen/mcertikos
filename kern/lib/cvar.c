#include <lib/debug.h>
#include <lib/gcc.h>
#include <lib/stdarg.h>
#include <lib/x86.h>

#include <lib/types.h>
#include <lib/spinlock.h>

/**
 *  class CV {   
 *      private Queue waiting;   
 *      public void wait(Lock *lock);   
 *      public void signal();   
 *      public void broadcast(); 
 *  } 
 * 
 *  // Monitor lock held by current thread. 
 *  void CV::wait(Lock *lock) {     
 *      assert(lock.isHeld()); 
 *      waiting.add(myTCB);  
 *      // Switch to new thread & release lock. 
 *      scheduler.suspend(&lock);     
 *      lock->acquire(); 
 *  } 
 * 
 *  // Monitor lock held by current thread. 
 *  void CV::signal() {     
 *      if (waiting.notEmpty()) {         
 *          thread = waiting.remove(); 
 *          scheduler.makeReady(thread);     
 *      } 
 *  } 
 * 
 *  void CV::broadcast() {     
 *      while (waiting.notEmpty()) {         
 *          thread = waiting.remove(); 
 *          scheduler.makeReady(thread);     
 *      } 
 *  }
*/