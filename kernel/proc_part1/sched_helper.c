
#include "globals.h"
#include "errno.h"
#include "main/interrupt.h"
#include "proc/sched.h"
#include "proc/kthread.h"
#include "util/init.h"
#include "util/debug.h"
void ktqueue_enqueue(ktqueue_t *q, kthread_t *thr);
kthread_t * ktqueue_dequeue(ktqueue_t *q);

/*
 * Updates the thread's state and enqueues it on the given
 * queue. Returns when the thread has been woken up with wakeup_on or
 * broadcast_on.
 *
 * Use the private queue manipulation functions above.
 */
void
sched_sleep_on(ktqueue_t *q)
{
        // NOT_YET_IMPLEMENTED("PROCS: sched_sleep_on");
		curthr->kt_state=KT_SLEEP;
        ktqueue_enqueue(q,curthr);
        sched_switch();
}

kthread_t *
sched_wakeup_on(ktqueue_t *q)
{
		kthread_t *wakeThread;
        // NOT_YET_IMPLEMENTED("PROCS: sched_wakeup_on");
		if(sched_queue_empty(q))
		{//empty queue
			return NULL;
		}
		else//not empty: can perform the wakeup
		{

			wakeThread=ktqueue_dequeue(q);
			sched_make_runnable(wakeThread);
			return wakeThread;
		}
		
}

void
sched_broadcast_on(ktqueue_t *q)
{
        // NOT_YET_IMPLEMENTED("PROCS: sched_broadcast_on");
        while(!sched_queue_empty(q))
			{
				sched_wakeup_on(q);
			}
}


