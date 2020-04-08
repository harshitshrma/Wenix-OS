
#include "types.h"
#include "globals.h"
#include "kernel.h"
#include "errno.h"

#include "util/debug.h"

#include "proc/proc.h"

#include "mm/mm.h"
#include "mm/mman.h"
#include "mm/page.h"
#include "mm/mmobj.h"
#include "mm/pframe.h"
#include "mm/pagetable.h"

#include "vm/pagefault.h"
#include "vm/vmmap.h"
#include "api/access.h"

/*
 * This gets called by _pt_fault_handler in mm/pagetable.c The
 * calling function has already done a lot of error checking for
 * us. In particular it has checked that we are not page faulting
 * while in kernel mode. Make sure you understand why an
 * unexpected page fault in kernel mode is bad in Weenix. You
 * should probably read the _pt_fault_handler function to get a
 * sense of what it is doing.
 *
 * Before you can do anything you need to find the vmarea that
 * contains the address that was faulted on. Make sure to check
 * the permissions on the area to see if the process has
 * permission to do [cause]. If either of these checks does not
 * pass kill the offending process, setting its exit status to
 * EFAULT (normally we would send the SIGSEGV signal, however
 * Weenix does not support signals).
 *
 * Now it is time to find the correct page. Make sure that if the
 * user writes to the page it will be handled correctly. This
 * includes your shadow objects' copy-on-write magic working
 * correctly.
 *
 * Finally call pt_map to have the new mapping placed into the
 * appropriate page table.
 *
 * @param vaddr the address that was accessed to cause the fault
 *
 * @param cause this is the type of operation on the memory
 *              address which caused the fault, possible values
 *              can be found in pagefault.h
 */
void
handle_pagefault(uintptr_t vaddr, uint32_t cause)
{
        //NOT_YET_IMPLEMENTED("VM: handle_pagefault");
		dbg(DBG_TEMP, "handle_pagefault entry\n");
        pframe_t *pf;
        vmmap_t *map =  curproc->p_vmmap;
        vmarea_t *new = vmmap_lookup(map, ADDR_TO_PN(vaddr));
        int forwrite = 0;
        int perm = 0;
        int res = 0;

        if(new==NULL)
        {	
        	dbg(DBG_TEMP, "handle_pagefault exit\n");
        	dbg(DBG_PRINT, "(GRADING3C 5)\n");
            proc_kill(curproc,EFAULT);
        }
        if(new->vma_prot & PROT_NONE)
        {
        	dbg(DBG_TEMP, "handle_pagefault exit\n");
        	dbg(DBG_PRINT, "(GRADING3B X)\n");
            proc_kill(curproc,EFAULT);
        }
        if ((cause & FAULT_WRITE) && !(new->vma_prot & PROT_WRITE))
        {
        	dbg(DBG_TEMP, "handle_pagefault exit\n");
        	dbg(DBG_PRINT, "(GRADING3B X)\n");
            proc_kill(curproc,EFAULT);
        }
        if ((cause & FAULT_EXEC) && !(new->vma_prot & PROT_EXEC))
        {
        	dbg(DBG_TEMP, "handle_pagefault exit\n");
        	dbg(DBG_PRINT, "(GRADING3B X)\n");
            proc_kill(curproc,EFAULT);
        }
        if(!(cause & FAULT_WRITE) && !(cause & FAULT_EXEC) && !(new->vma_prot & PROT_READ))
        {
        	dbg(DBG_TEMP, "handle_pagefault exit\n");
        	dbg(DBG_PRINT, "(GRADING3B X)\n");
            proc_kill(curproc,EFAULT);
        }

        uint32_t pdflags = PD_PRESENT | PD_USER;
        uint32_t ptflags = PT_PRESENT | PT_USER;


        if(cause & FAULT_WRITE)
        {
            forwrite=1;
            pdflags |= PD_WRITE;
            ptflags |= PT_WRITE;
        }
        if (new->vma_obj->mmo_shadowed)
        {
        	res = new->vma_obj->mmo_ops->lookuppage(new->vma_obj, ADDR_TO_PN(vaddr)-new->vma_start+new->vma_off, forwrite, &pf);
        }
        else
        {
	        res = pframe_lookup(new->vma_obj, ADDR_TO_PN(vaddr)-new->vma_start+new->vma_off,forwrite, &pf);
        }
        KASSERT(pf);
        KASSERT(pf->pf_addr);
        dbg(DBG_PRINT, "(GRADING3A 5.a)\n");
        if (res < 0)
        {
            //curproc->p_status = res;
            dbg(DBG_TEMP, "handle_pagefault exit\n");
            dbg(DBG_PRINT, "(GRADING3B X)\n");
            proc_kill(curproc,EFAULT);
        }
        if(forwrite)
        {
            int error = pframe_dirty(pf);
            if(error<0)
            {
              dbg(DBG_TEMP, "handle_pagefault exit\n");
              dbg(DBG_PRINT, "(GRADING3B X)\n");
              proc_kill(curproc,EFAULT);  
            }

        }
        pt_map(curproc->p_pagedir, (uintptr_t)PAGE_ALIGN_DOWN(vaddr), pt_virt_to_phys((uintptr_t)pf->pf_addr), pdflags, ptflags);
        dbg(DBG_PRINT, "(GRADING3B 1)\n");
        dbg(DBG_TEMP, "handle_pagefault exit\n");

}
