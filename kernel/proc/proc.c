
#include "kernel.h"
#include "config.h"
#include "globals.h"
#include "errno.h"

#include "util/debug.h"
#include "util/list.h"
#include "util/string.h"
#include "util/printf.h"

#include "proc/kthread.h"
#include "proc/proc.h"
#include "proc/sched.h"
#include "proc/proc.h"

#include "mm/slab.h"
#include "mm/page.h"
#include "mm/mmobj.h"
#include "mm/mm.h"
#include "mm/mman.h"

#include "vm/vmmap.h"

#include "fs/vfs.h"
#include "fs/vfs_syscall.h"
#include "fs/vnode.h"
#include "fs/file.h"

proc_t *curproc = NULL; /* global */
static slab_allocator_t *proc_allocator = NULL;

static list_t _proc_list;
static proc_t *proc_initproc = NULL; /* Pointer to the init process (PID 1) */

void
proc_init()
{
        list_init(&_proc_list);
        proc_allocator = slab_allocator_create("proc", sizeof(proc_t));
        KASSERT(proc_allocator != NULL);
}

proc_t *
proc_lookup(int pid)
{
        proc_t *p;
        list_iterate_begin(&_proc_list, p, proc_t, p_list_link) {
                if (p->p_pid == pid) {
                        return p;
                }
        } list_iterate_end();
        return NULL;
}

list_t *
proc_list()
{
        return &_proc_list;
}

size_t
proc_info(const void *arg, char *buf, size_t osize)
{
        const proc_t *p = (proc_t *) arg;
        size_t size = osize;
        proc_t *child;

        KASSERT(NULL != p);
        KASSERT(NULL != buf);

        iprintf(&buf, &size, "pid:          %i\n", p->p_pid);
        iprintf(&buf, &size, "name:         %s\n", p->p_comm);
        if (NULL != p->p_pproc) {
                iprintf(&buf, &size, "parent:       %i (%s)\n",
                        p->p_pproc->p_pid, p->p_pproc->p_comm);
        } else {
                iprintf(&buf, &size, "parent:       -\n");
        }

#ifdef __MTP__
        int count = 0;
        kthread_t *kthr;
        list_iterate_begin(&p->p_threads, kthr, kthread_t, kt_plink) {
                ++count;
        } list_iterate_end();
        iprintf(&buf, &size, "thread count: %i\n", count);
#endif

        if (list_empty(&p->p_children)) {
                iprintf(&buf, &size, "children:     -\n");
        } else {
                iprintf(&buf, &size, "children:\n");
        }
        list_iterate_begin(&p->p_children, child, proc_t, p_child_link) {
                iprintf(&buf, &size, "     %i (%s)\n", child->p_pid, child->p_comm);
        } list_iterate_end();

        iprintf(&buf, &size, "status:       %i\n", p->p_status);
        iprintf(&buf, &size, "state:        %i\n", p->p_state);

#ifdef __VFS__
#ifdef __GETCWD__
        if (NULL != p->p_cwd) {
                char cwd[256];
                lookup_dirpath(p->p_cwd, cwd, sizeof(cwd));
                iprintf(&buf, &size, "cwd:          %-s\n", cwd);
        } else {
                iprintf(&buf, &size, "cwd:          -\n");
        }
#endif /* __GETCWD__ */
#endif

#ifdef __VM__
        iprintf(&buf, &size, "start brk:    0x%p\n", p->p_start_brk);
        iprintf(&buf, &size, "brk:          0x%p\n", p->p_brk);
#endif

        return size;
}

size_t
proc_list_info(const void *arg, char *buf, size_t osize)
{
        size_t size = osize;
        proc_t *p;

        KASSERT(NULL == arg);
        KASSERT(NULL != buf);

#if defined(__VFS__) && defined(__GETCWD__)
        iprintf(&buf, &size, "%5s %-13s %-18s %-s\n", "PID", "NAME", "PARENT", "CWD");
#else
        iprintf(&buf, &size, "%5s %-13s %-s\n", "PID", "NAME", "PARENT");
#endif

        list_iterate_begin(&_proc_list, p, proc_t, p_list_link) {
                char parent[64];
                if (NULL != p->p_pproc) {
                        snprintf(parent, sizeof(parent),
                                 "%3i (%s)", p->p_pproc->p_pid, p->p_pproc->p_comm);
                } else {
                        snprintf(parent, sizeof(parent), "  -");
                }

#if defined(__VFS__) && defined(__GETCWD__)
                if (NULL != p->p_cwd) {
                        char cwd[256];
                        lookup_dirpath(p->p_cwd, cwd, sizeof(cwd));
                        iprintf(&buf, &size, " %3i  %-13s %-18s %-s\n",
                                p->p_pid, p->p_comm, parent, cwd);
                } else {
                        iprintf(&buf, &size, " %3i  %-13s %-18s -\n",
                                p->p_pid, p->p_comm, parent);
                }
#else
                iprintf(&buf, &size, " %3i  %-13s %-s\n",
                        p->p_pid, p->p_comm, parent);
#endif
        } list_iterate_end();
        return size;
}

static pid_t next_pid = 0;

/**
 * Returns the next available PID.
 *
 * Note: Where n is the number of running processes, this algorithm is
 * worst case O(n^2). As long as PIDs never wrap around it is O(n).
 *
 * @return the next available PID
 */
static int
_proc_getid()
{
        proc_t *p;
        pid_t pid = next_pid;
        while (1) {
failed:
                list_iterate_begin(&_proc_list, p, proc_t, p_list_link) {
                        if (p->p_pid == pid) {
                                if ((pid = (pid + 1) % PROC_MAX_COUNT) == next_pid) {
                                        return -1;
                                } else {
                                        goto failed;
                                }
                        }
                } list_iterate_end();
                next_pid = (pid + 1) % PROC_MAX_COUNT;
                return pid;
        }
}

/*
 * The new process, although it isn't really running since it has no
 * threads, should be in the PROC_RUNNING state.
 *
 * Don't forget to set proc_initproc when you create the init
 * process. You will need to be able to reference the init process
 * when reparenting processes to the init process.
 */
proc_t *
proc_create(char *name)
{
        /*NOT_YET_IMPLEMENTED("PROCS: proc_create");*/
        pid_t p_num = _proc_getid();//Get next available process id

        proc_t *process = (proc_t *)slab_obj_alloc(proc_allocator); //Allocate an object of the slab as a PCB
        memset(process,0,sizeof(proc_t));//Initialize memory

        strcpy(process->p_comm,name);//Copy the name of the process
        process->p_pid = p_num;//Assign Process ID
        process->p_state = PROC_RUNNING;//Set the state of the process
        process->p_status = 0;//Set the status of the process
        process->p_cwd = vfs_root_vn;
        process->p_vmmap = vmmap_create();
        process->p_vmmap->vmm_proc = process;
        //vref(vfs_root_vn);

        list_init(&process->p_threads);//Initialize the thread list
        list_init(&process->p_children);//Initialze the children list
        sched_queue_init(&process->p_wait);//Initialze Wait queue
        process->p_pagedir = pt_create_pagedir();//Initialize Page Directory
        list_link_init(&process->p_list_link);//Initialize Process Link
        list_link_init(&process->p_child_link);//Initialize Child Link
        KASSERT(PID_IDLE != process->p_pid || list_empty(&_proc_list)); 
        KASSERT(PID_INIT != process->p_pid || PID_IDLE == curproc->p_pid);
        dbg(DBG_PRINT, "(GRADING1A 2.a)\n");
        list_insert_tail(&_proc_list, &process->p_list_link); 
        
        if (process->p_pid == PID_IDLE)
        {
            process->p_pproc = NULL;
            dbg(DBG_PRINT, "(GRADING1A 1.a)\n");
        }
        else if(process->p_pid == PID_INIT)
        { 
            process->p_pproc = curproc;
            proc_initproc = process;
            list_insert_tail(&curproc->p_children, &process->p_child_link);
            dbg(DBG_PRINT, "(GRADING1A 1.b)\n");
        }
        else
        {
            process->p_pproc = curproc;
            list_insert_tail(&curproc->p_children, &process->p_child_link);
            dbg(DBG_PRINT, "(GRADING1C)\n");
        }
        return process;//Return the process
}

/**
 * Cleans up as much as the process as can be done from within the
 * process. This involves:
 *    - Closing all open files (VFS)
 *    - Cleaning up VM mappings (VM)
 *    - Waking up its parent if it is waiting
 *    - Reparenting any children to the init process
 *    - Setting its status and state appropriately
 *
 * The parent will finish destroying the process within do_waitpid (make
 * sure you understand why it cannot be done here). Until the parent
 * finishes destroying it, the process is informally called a 'zombie'
 * process.
 *
 * This is also where any children of the current process should be
 * reparented to the init process (unless, of course, the current
 * process is the init process. However, the init process should not
 * have any children at the time it exits).
 *
 * Note: You do _NOT_ have to special case the idle process. It should
 * never exit this way.
 *
 * @param status the status to exit the process with
 */
void
proc_cleanup(int status)
{
        /*NOT_YET_IMPLEMENTED("PROCS: proc_cleanup");*/
        KASSERT(NULL != proc_initproc);
        KASSERT(1 <= curproc->p_pid);
        KASSERT(NULL != curproc->p_pproc); 
        dbg(DBG_PRINT, "(GRADING1A 2.b)\n");
        if(curproc->p_state != PROC_DEAD && curproc->p_pid != PID_IDLE)
        {
            dbg(DBG_PRINT, "(GRADING1C)\n");
            if (sched_queue_empty(&curproc->p_pproc->p_wait) == 0)
            {
                dbg(DBG_PRINT, "(GRADING1C)\n");
                sched_wakeup_on(&curproc->p_pproc->p_wait);
            }
            if (list_empty(&curproc->p_children) == 0 && curproc->p_pid!=PID_INIT)
            {
                proc_t *p;
                list_iterate_begin(&curproc->p_children, p, proc_t, p_child_link)
                {	
                	p->p_pproc=proc_initproc;
                    list_insert_tail(&proc_initproc->p_children, &p->p_child_link);
                    dbg(DBG_PRINT, "(GRADING1C)\n");
                } 
                list_iterate_end();
                dbg(DBG_PRINT, "(GRADING1C)\n");
            }
            curproc->p_state = PROC_DEAD;
            curproc->p_status = status;
            list_remove(&curproc->p_list_link);
            vmmap_destroy(curproc->p_vmmap);
            for (int i = 0; i < NFILES; i++)
            {
                if (curproc->p_files[i] != NULL)
                {
                    if (curproc->p_files[i]->f_vnode->vn_refcount != 0 && curproc->p_files[i]->f_vnode->vn_refcount > curproc->p_files[i]->f_vnode->vn_nrespages)
                    {
                        do_close(i);
                    }
                }
            }
            KASSERT(NULL != curproc->p_pproc); 
            KASSERT(KT_EXITED == curthr->kt_state); 
            dbg(DBG_PRINT, "(GRADING1A 2.b)\n");
            sched_switch();
        }
        dbg(DBG_PRINT, "(GRADING1C)\n");
}

/*
 * This has nothing to do with signals and kill(1).
 *
 * Calling this on the current process is equivalent to calling
 * do_exit().
 *
 * In Wenix, this is only called from proc_kill_all.
 */
void
proc_kill(proc_t *p, int status)
{
        /*NOT_YET_IMPLEMENTED("PROCS: proc_kill");*/
        if (p == curproc)
        {
            dbg(DBG_PRINT, "(GRADING1C)\n");
            do_exit(status);
        }
        else
        { 
            kthread_t *thread;
            list_iterate_begin(&p->p_threads, thread, kthread_t, kt_plink) 
            {
                if (thread->kt_state != KT_SLEEP)
                {
                    kthread_cancel(thread, (void *)status);
                    dbg(DBG_PRINT, "(GRADING1C)\n");
                }
                dbg(DBG_PRINT, "(GRADING1C)\n");
            }
            list_iterate_end();
            dbg(DBG_PRINT, "(GRADING1C)\n");
        } 
        dbg(DBG_PRINT, "(GRADING1C)\n");
}

/*
 * Remember, proc_kill on the current process will _NOT_ return.
 * Don't kill direct children of the idle process.
 *
 * In Wenix, this is only called by sys_halt.
 */
void
proc_kill_all()
{
        /*NOT_YET_IMPLEMENTED("PROCS: proc_kill_all");*/
        proc_t *proc;
        list_iterate_begin(&_proc_list, proc, proc_t, p_list_link)
        {
            dbg(DBG_PRINT, "(GRADING1C)\n");
            if (proc != curproc)
            {
                if(proc->p_pproc!=NULL)
                {
                    dbg(DBG_PRINT, "(GRADING1C)\n");
                    if(proc->p_pproc->p_pid != PID_IDLE && proc->p_pid != PID_IDLE)
                    {
                        dbg(DBG_PRINT, "(GRADING1C)\n");
                        proc_kill(proc,1);
                    }
                }
            }
            
        } 
        list_iterate_end();
            

        dbg(DBG_PRINT, "(GRADING1C)\n");
}

/*
 * This function is only called from kthread_exit.
 *
 * Unless you are implementing MTP, this just means that the process
 * needs to be cleaned up and a new thread needs to be scheduled to
 * run. If you are implementing MTP, a single thread exiting does not
 * necessarily mean that the process should be exited.
 */
void
proc_thread_exited(void *retval)
{
        /*NOT_YET_IMPLEMENTED("PROCS: proc_thread_exited");*/
        dbg(DBG_PRINT, "(GRADING1C)\n");
        proc_cleanup((int)retval);
}

/* If pid is -1 dispose of one of the exited children of the current
 * process and return its exit status in the status argument, or if
 * all children of this process are still running, then this function
 * blocks on its own p_wait queue until one exits.
 *
 * If pid is greater than 0 and the given pid is a child of the
 * current process then wait for the given pid to exit and dispose
 * of it.
 *
 * If the current process has no children, or the given pid is not
 * a child of the current process return -ECHILD.
 *
 * Pids other than -1 and positive numbers are not supported.
 * Options other than 0 are not supported.
 */
pid_t
do_waitpid(pid_t pid, int options, int *status)
{
        //NOT_YET_IMPLEMENTED("PROCS: do_waitpid");
        pid_t temp_pid;
        proc_t *temp_proc;
        if (list_empty(&curproc->p_children) == 1)
        {
            dbg(DBG_PRINT, "(GRADING1C)\n");
            return -ECHILD;
        }
        if (pid == -1)
        {
            dbg(DBG_PRINT, "(GRADING1C)\n");
            check:
            list_iterate_begin(&curproc->p_children, temp_proc, proc_t, p_child_link)
            {
                    if (temp_proc->p_state == PROC_DEAD)
                    {
                        KASSERT(NULL != temp_proc);
                        KASSERT(-1 == pid || temp_proc->p_pid == pid);
                        dbg(DBG_PRINT, "(GRADING1A 2.c)\n");
                        kthread_t *new_thread = list_item(&temp_proc->p_threads.l_next, kthread_t,kt_plink);
                        if (new_thread->kt_state == KT_EXITED)
                        {
                            dbg(DBG_PRINT, "(GRADING1C)\n");
                            kthread_destroy(new_thread);
                        }
                        if (status != NULL)
                        {
                            *status = temp_proc->p_status;
                        }
                        temp_pid = temp_proc->p_pid;
                        list_remove(&temp_proc->p_child_link);
                        KASSERT(NULL != temp_proc->p_pagedir);
                        dbg(DBG_PRINT, "(GRADING1A 2.c)\n");
                        pt_destroy_pagedir(temp_proc->p_pagedir);
                        slab_obj_free(proc_allocator, temp_proc);
                        dbg(DBG_PRINT, "(GRADING1C)\n");
                        return temp_pid;
                    }                    
            } 
            list_iterate_end();
            sched_sleep_on(&curproc->p_wait);
            dbg(DBG_PRINT, "(GRADING1C)\n");
            goto check;
        }
        if (pid > 0)
        {
            dbg(DBG_PRINT, "(GRADING1C)\n");
            list_iterate_begin(&curproc->p_children, temp_proc, proc_t, p_child_link)
            {
                if (temp_proc->p_pid == pid)
                {
                    dbg(DBG_PRINT, "(GRADING1C)\n");
                    check1:
                    if (temp_proc->p_state != PROC_DEAD)
                    {
                        dbg(DBG_PRINT, "(GRADING1C)\n");
                        sched_sleep_on(&curproc->p_wait);
                    }
                    else
                    {
                        KASSERT(NULL != temp_proc);
                        KASSERT(-1 == pid || temp_proc->p_pid == pid);
                        dbg(DBG_PRINT, "(GRADING1A 2.c)\n");
                        kthread_t *new_thread_1 = list_item(&temp_proc->p_threads.l_next, kthread_t,kt_plink);
                        if (new_thread_1->kt_state == KT_EXITED)
                        {
                            dbg(DBG_PRINT, "(GRADING1C)\n");
                            kthread_destroy(new_thread_1);
                        }
                        if (status != NULL)
                        {
                            *status = temp_proc->p_status;
                        }
                        temp_pid = temp_proc->p_pid;
                        list_remove(&temp_proc->p_child_link);
                        KASSERT(NULL != temp_proc->p_pagedir);
                        dbg(DBG_PRINT, "(GRADING1A 2.c)\n");
                        pt_destroy_pagedir(temp_proc->p_pagedir);
                        slab_obj_free(proc_allocator, temp_proc);
                        dbg(DBG_PRINT, "(GRADING1C)\n");
                        return temp_pid;
                    }
                    dbg(DBG_PRINT, "(GRADING1C)\n");
                    goto check1;
                }
                else
                {
                    dbg(DBG_PRINT, "(GRADING1C)\n");
                    return -ECHILD;
                }
            } 
            list_iterate_end();
        }
        dbg(DBG_PRINT, "(GRADING1C)\n");
        return 0;
}

/*
 * Cancel all threads and join with them (if supporting MTP), and exit from the current
 * thread.
 *
 * @param status the exit status of the process
 */
void
do_exit(int status)
{
        /*NOT_YET_IMPLEMENTED("PROCS: do_exit");*/
        dbg(DBG_PRINT, "(GRADING1C)\n");
        kthread_exit((void *)status);
}
