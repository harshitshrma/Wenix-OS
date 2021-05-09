
#include "types.h"
#include "globals.h"
#include "errno.h"

#include "util/debug.h"
#include "util/string.h"

#include "proc/proc.h"
#include "proc/kthread.h"

#include "mm/mm.h"
#include "mm/mman.h"
#include "mm/page.h"
#include "mm/pframe.h"
#include "mm/mmobj.h"
#include "mm/pagetable.h"
#include "mm/tlb.h"

#include "fs/file.h"
#include "fs/vnode.h"

#include "vm/shadow.h"
#include "vm/vmmap.h"

#include "api/exec.h"

#include "main/interrupt.h"

/* Pushes the appropriate things onto the kernel stack of a newly forked thread
 * so that it can begin execution in userland_entry.
 * regs: registers the new thread should have on execution
 * kstack: location of the new thread's kernel stack
 * Returns the new stack pointer on success. */
static uint32_t
fork_setup_stack(const regs_t *regs, void *kstack)
{
        /* Pointer argument and dummy return address, and userland dummy return
         * address */
        uint32_t esp = ((uint32_t) kstack) + DEFAULT_STACK_SIZE - (sizeof(regs_t) + 12);
        *(void **)(esp + 4) = (void *)(esp + 8); /* Set the argument to point to location of struct on stack */
        memcpy((void *)(esp + 8), regs, sizeof(regs_t)); /* Copy over struct */
        return esp;
}


/*
 * The implementation of fork(2). Once this works,
 * you're practically home free. This is what the
 * entirety of Wenix has been leading up to.
 * Go forth and conquer.
 */
int
do_fork(struct regs *regs)
{
        /*vmarea_t *vma, *clone_vma;
        pframe_t *pf;
        mmobj_t *to_delete, *new_shadowed;
        //NOT_YET_IMPLEMENTED("VM: do_fork");
        KASSERT(regs != NULL);
        KASSERT(curproc != NULL);
        KASSERT(curproc->p_state == PROC_RUNNING);
        dbg(DBG_PRINT, "(GRADING3A 7.a)\n");

        proc_t *fork_process=proc_create("fork_process");

        KASSERT(fork_process->p_state == PROC_RUNNING);
        KASSERT(fork_process->p_pagedir != NULL);
        dbg(DBG_PRINT, "(GRADING3A 7.a)\n");

        fork_process->p_vmmap =vmmap_clone(curproc->p_vmmap);
        fork_process->p_vmmap->vmm_proc = fork_process;

        list_t *old_list = &curproc->p_vmmap->vmm_list;
        list_t *new_list = &fork_process->p_vmmap->vmm_list;
    


        list_iterate_begin(old_list,vma,vmarea_t,vma_plink)
        {
            //vma =list_item(&temp_list, vmarea_t, vma_plink);
            clone_vma = vmmap_lookup(fork_process->p_vmmap, vma->vma_start);
            
            if (vma->vma_flags == MAP_PRIVATE)
            {
                to_delete = shadow_create();
                to_delete->mmo_shadowed = vma->vma_obj;
                to_delete->mmo_un.mmo_bottom_obj = vma->vma_obj->mmo_un.mmo_bottom_obj;
                vma->vma_obj->mmo_un.mmo_bottom_obj->mmo_ops->ref(vma->vma_obj->mmo_un.mmo_bottom_obj);
                //vma->vma_obj = to_delete;  
                //to_delete->mmo_ops->ref(to_delete);

                new_shadowed = shadow_create();
                new_shadowed->mmo_shadowed = vma->vma_obj;
                new_shadowed->mmo_un.mmo_bottom_obj = vma->vma_obj->mmo_un.mmo_bottom_obj;
                vma->vma_obj->mmo_un.mmo_bottom_obj->mmo_ops->ref(vma->vma_obj->mmo_un.mmo_bottom_obj);
                //new_shadowed->mmo_un.mmo_bottom_obj->mmo_ops->ref(new_shadowed->mmo_un.mmo_bottom_obj);
                //clone_vma->vma_obj = new_shadowed;
                //new_shadowed->mmo_ops->ref(new_shadowed); 
                list_insert_tail(&vma->vma_obj->mmo_un.mmo_bottom_obj->mmo_un.mmo_vmas, &clone_vma->vma_olink);
                //vma->vma_obj->mmo_ops->ref(vma->vma_obj);
                clone_vma->vma_obj = new_shadowed;
                vma->vma_obj = to_delete;
                new_shadowed->mmo_ops->ref(new_shadowed);
                to_delete->mmo_ops->ref(to_delete);
            }
            else
            {
                //clone_vma->vma_obj = vma->vma_obj;
                //list_insert_tail(&(vma->vma_obj->mmo_un.mmo_vmas),&clone_vma->vma_olink);
                //clone_vma->vma_obj->mmo_ops->ref(clone_vma->vma_obj);
            }  
            //temp_list = temp_list->l_next;
        }
        list_iterate_end();


        kthread_t *temp_thread;
        kthread_t *child_thread; //= kthread_create(fork_process, NULL, 0, NULL);
        
        list_iterate_begin(&curproc->p_threads, temp_thread, kthread_t,kt_plink)
        {
            child_thread = kthread_clone(temp_thread);

            KASSERT(child_thread->kt_kstack != NULL);
            dbg(DBG_PRINT, "(GRADING3A 7.a)\n");

            child_thread->kt_proc = fork_process; 
            list_insert_tail(&fork_process->p_threads, &child_thread->kt_plink);

            child_thread->kt_ctx.c_pdptr=fork_process->p_pagedir;
            child_thread->kt_ctx.c_eip = (uint32_t)userland_entry;
            child_thread->kt_ctx.c_esp = fork_setup_stack(regs, child_thread->kt_kstack);
            regs->r_eax = 0;
            //child_thread->kt_ctx.c_ebp = curthr->kt_ctx.c_ebp;
            //child_thread->kt_ctx.c_kstack = (uint32_t)child_thread->kt_kstack;
            //child_thread->kt_ctx.c_kstacksz = DEFAULT_STACK_SIZE;
            //if (temp_thread == curthr)
            //{
                
            //}
        
        }
        list_iterate_end();
        
        for (int i = 0; i < NFILES; i++)
        {
            fork_process->p_files[i] = curproc->p_files[i];
            if(curproc->p_files[i])
            {
                fref(curproc->p_files[i]);
            }
        }


       
        tlb_flush_all();
        pt_unmap_range(curproc->p_pagedir, USER_MEM_LOW, USER_MEM_HIGH);
        
        //fref(fork_process->p_files[NFILES]);
        fork_process->p_cwd = curproc->p_cwd;
        ///fork_process->p_pproc = curproc;
        fork_process->p_brk = curproc->p_brk;
        fork_process->p_start_brk = curproc->p_start_brk;
        //vref(fork_process->p_cwd);
        sched_make_runnable(child_thread);
        return fork_process->p_pid;*/
        dbg(DBG_TEMP, "do_fork entry\n");
        vmarea_t *vma, *clone_vma;
        pframe_t *pf;
        mmobj_t *to_delete, *new_shadowed;
        //NOT_YET_IMPLEMENTED("VM: do_fork");
        KASSERT(regs != NULL);
        KASSERT(curproc != NULL);
        KASSERT(curproc->p_state == PROC_RUNNING);
        dbg(DBG_PRINT, "(GRADING3A 7.a)\n");

        
        proc_t *fork_process=proc_create("fork_process");
        KASSERT(fork_process->p_state == PROC_RUNNING);
        KASSERT(fork_process->p_pagedir != NULL);
        dbg(DBG_PRINT, "(GRADING3A 7.a)\n");
        //list_insert_tail(&curproc->p_children, &fork_process->p_child_link);
        //curthr->kt_retval = (int *)fork_process->p_pid;
        fork_process->p_vmmap =vmmap_clone(curproc->p_vmmap);
        list_iterate_begin(&fork_process->p_vmmap->vmm_list,clone_vma,vmarea_t,vma_plink)
        {
            //list_iterate_begin(&curproc->p_vmmap->vmm_list,vma, vmarea_t, vma_plink)
            //{
            vma = vmmap_lookup(curproc->p_vmmap, clone_vma->vma_start);
                if ((vma->vma_flags & MAP_PRIVATE) ==  MAP_PRIVATE)
                {
                    to_delete = shadow_create();
                    to_delete->mmo_shadowed = vma->vma_obj;
                    to_delete->mmo_un.mmo_bottom_obj = vma->vma_obj->mmo_un.mmo_bottom_obj;
                    //vma->vma_obj->mmo_un.mmo_bottom_obj->mmo_ops->ref(vma->vma_obj->mmo_un.mmo_bottom_obj);
                    //vma->vma_obj = to_delete;  
                    //to_delete->mmo_ops->ref(to_delete);

                    new_shadowed = shadow_create();
                    new_shadowed->mmo_shadowed = vma->vma_obj;
                    new_shadowed->mmo_un.mmo_bottom_obj = vma->vma_obj->mmo_un.mmo_bottom_obj;
                    //vma->vma_obj->mmo_un.mmo_bottom_obj->mmo_ops->ref(vma->vma_obj->mmo_un.mmo_bottom_obj);
                    //new_shadowed->mmo_un.mmo_bottom_obj->mmo_ops->ref(new_shadowed->mmo_un.mmo_bottom_obj);
                    //clone_vma->vma_obj = new_shadowed;
                    //new_shadowed->mmo_ops->ref(new_shadowed); 
                    //list_insert_tail(&vma->vma_obj->mmo_un.mmo_bottom_obj->mmo_un.mmo_vmas, &clone_vma->vma_olink);

                    clone_vma->vma_obj = new_shadowed;
                    vma->vma_obj = to_delete;
                    vma->vma_obj->mmo_ops->ref(vma->vma_obj);
                    //clone_vma->vma_obj->mmo_ops->ref(clone_vma->vma_obj);
                    //list_insert_tail(&vma->vma_obj->mmo_un.mmo_bottom_obj->mmo_un.mmo_vmas, &clone_vma->vma_olink);
                    //list_insert_tail(&vma->vma_obj->mmo_un.mmo_bottom_obj->mmo_un.mmo_vmas, &vma->vma_olink);
                    //new_shadowed->mmo_ops->ref(new_shadowed);
                    //to_delete->mmo_ops->ref(to_delete); 
                    //clone_vma->vma_obj->mmo_ops->ref(clone_vma->vma_obj);

                    pt_unmap_range(curproc->p_pagedir,  (uintptr_t)PN_TO_ADDR(vma->vma_start), (uintptr_t)PN_TO_ADDR(vma->vma_end)); 
                    tlb_flush_all();
                    dbg(DBG_PRINT, "(GRADING3B 7)\n");
                }
                else
                {
                    clone_vma->vma_obj = vma->vma_obj;
                    clone_vma->vma_obj->mmo_ops->ref(clone_vma->vma_obj);
                    dbg(DBG_PRINT, "(GRADING3B X)\n");
                }
            //}
            //list_iterate_end();
        dbg(DBG_PRINT, "(GRADING3B 7)\n");        
        }
        list_iterate_end();
        
        
        kthread_t *temp_thread, *child_thread;
        list_iterate_begin(&curproc->p_threads, temp_thread, kthread_t,kt_plink)
        {
            child_thread = kthread_clone(temp_thread);
            child_thread->kt_proc = fork_process; 
            list_insert_tail(&fork_process->p_threads, &child_thread->kt_plink);
            child_thread->kt_ctx.c_pdptr = fork_process->p_pagedir;
            child_thread->kt_ctx.c_eip = (uint32_t)userland_entry;
            regs->r_eax = 0;
            child_thread->kt_ctx.c_esp = fork_setup_stack(regs, child_thread->kt_kstack);
            //child_thread->kt_ctx.c_ebp=curthr->kt_ctx.c_ebp;
            child_thread->kt_ctx.c_kstacksz = DEFAULT_STACK_SIZE;
            child_thread->kt_ctx.c_kstack = (uintptr_t)child_thread->kt_kstack;
            KASSERT(child_thread->kt_kstack != NULL);
            dbg(DBG_PRINT, "(GRADING3A 7.a)\n");
            dbg(DBG_PRINT, "(GRADING3B 7)\n");
        }
        list_iterate_end();
        for (int i = 0; i < NFILES; i++)
        {
            fork_process->p_files[i] = curproc->p_files[i];
            if(curproc->p_files[i])
            {
                fref(curproc->p_files[i]);
                dbg(DBG_PRINT, "(GRADING3B 7)\n");
            }
            dbg(DBG_PRINT, "(GRADING3B 7)\n");
        }
        
        fork_process->p_cwd = curproc->p_cwd;
        //vref(curproc->p_cwd);
        fork_process->p_brk=curproc->p_brk;
        fork_process->p_start_brk=curproc->p_start_brk;
        sched_make_runnable(child_thread);
        //vget(fork_process->p_cwd->vn_fs, fork_process->p_cwd->vn_vno);
        dbg(DBG_TEMP, "do_fork exit\n");
        dbg(DBG_PRINT, "(GRADING3B 7)\n");
        return fork_process->p_pid;

}
