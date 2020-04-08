
#include "globals.h"
#include "errno.h"
#include "util/debug.h"

#include "mm/mm.h"
#include "mm/page.h"
#include "mm/mman.h"

#include "vm/mmap.h"
#include "vm/vmmap.h"

#include "proc/proc.h"

/*
 * This function implements the brk(2) system call.
 *
 * This routine manages the calling process's "break" -- the ending address
 * of the process's "dynamic" region (often also referred to as the "heap").
 * The current value of a process's break is maintained in the 'p_brk' member
 * of the proc_t structure that represents the process in question.
 *
 * The 'p_brk' and 'p_start_brk' members of a proc_t struct are initialized
 * by the loader. 'p_start_brk' is subsequently never modified; it always
 * holds the initial value of the break. Note that the starting break is
 * not necessarily page aligned!
 *
 * 'p_start_brk' is the lower limit of 'p_brk' (that is, setting the break
 * to any value less than 'p_start_brk' should be disallowed).
 *
 * The upper limit of 'p_brk' is defined by the minimum of (1) the
 * starting address of the next occuring mapping or (2) USER_MEM_HIGH.
 * That is, growth of the process break is limited only in that it cannot
 * overlap with/expand into an existing mapping or beyond the region of
 * the address space allocated for use by userland. (note the presence of
 * the 'vmmap_is_range_empty' function).
 *
 * The dynamic region should always be represented by at most ONE vmarea.
 * Note that vmareas only have page granularity, you will need to take this
 * into account when deciding how to set the mappings if p_brk or p_start_brk
 * is not page aligned.
 *
 * You are guaranteed that the process data/bss region is non-empty.
 * That is, if the starting brk is not page-aligned, its page has
 * read/write permissions.
 *
 * If addr is NULL, you should "return" the current break. We use this to
 * implement sbrk(0) without writing a separate syscall. Look in
 * user/libc/syscall.c if you're curious.
 *
 * You should support combined use of brk and mmap in the same process.
 *
 * Note that this function "returns" the new break through the "ret" argument.
 * Return 0 on success, -errno on failure.
 */
int
do_brk(void *addr, void **ret)
{
        //NOT_YET_IMPLEMENTED("VM: do_brk");
		dbg(DBG_TEMP, "do_brk entry\n");
        if(!addr)
        	{
        		*ret=curproc->p_brk;
        		dbg(DBG_TEMP, "do_brk exit\n");
        		dbg(DBG_PRINT, "(GRADING3C 2)\n");
        		return 0;
        	}
        if(addr<(curproc->p_start_brk) || (uint32_t)addr>USER_MEM_HIGH)
        	{
        		dbg(DBG_TEMP, "do_brk exit\n");
        		dbg(DBG_PRINT, "(GRADING3B X)\n");
				return -ENOMEM;
       	    }
		vmarea_t *new = vmmap_lookup(curproc->p_vmmap, ADDR_TO_PN(curproc->p_start_brk));
		if(new == NULL)
		{
			vmarea_t *new1;
			int res = do_mmap(curproc->p_start_brk,(size_t)(ADDR_TO_PN(addr) - ADDR_TO_PN(curproc->p_start_brk)), PROT_READ | PROT_WRITE , MAP_PRIVATE | MAP_ANON , -1 , 0 ,(void *)&new1);
			if(res)
			{
				curproc->p_brk = addr;
			    *ret = addr;
			    dbg(DBG_TEMP, "do_brk exit\n");
			    dbg(DBG_PRINT, "(GRADING3B X)\n");
	            return 0;
			}
			else
			{
				dbg(DBG_TEMP, "do_brk exit\n");
				dbg(DBG_PRINT, "(GRADING3B X)\n");
				return res;
			}
			dbg(DBG_PRINT, "(GRADING3B X)\n");
		}
		else
		{
			if(curproc->p_brk > addr)
			{
				vmmap_remove(curproc->p_vmmap, ADDR_TO_PN(addr)+1 ,ADDR_TO_PN(curproc->p_brk)-ADDR_TO_PN(addr));
				dbg(DBG_PRINT, "(GRADING3B X)\n");
			}
			else if(curproc->p_brk < addr)
			{
				vmarea_t *vm = vmmap_lookup(curproc->p_vmmap, ADDR_TO_PN(curproc->p_brk));
				int res = vmmap_is_range_empty(curproc->p_vmmap,vm->vma_end, ADDR_TO_PN(addr)-vm->vma_end + 1);
				if(res)
				{
					vm->vma_end = ADDR_TO_PN(addr) + 1;
					dbg(DBG_PRINT, "(GRADING3C 1)\n");
				}   
				else
				{
					dbg(DBG_TEMP, "do_brk exit\n");
					dbg(DBG_PRINT, "(GRADING3B X)\n");
					return -ENOMEM;
				}
				dbg(DBG_PRINT, "(GRADING3C 1)\n");
			}
			curproc->p_brk = addr;
		    *ret = addr;
		    dbg(DBG_TEMP, "do_brk exit\n");
		    dbg(DBG_PRINT, "(GRADING3C 1)\n");
	        return 0;	
		}
		
}		
