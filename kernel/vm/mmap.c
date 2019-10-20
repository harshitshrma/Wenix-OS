/******************************************************************************/
/* Important Spring 2019 CSCI 402 usage information:                          */
/*                                                                            */
/* This fils is part of CSCI 402 kernel programming assignments at USC.       */
/*         53616c7465645f5fd1e93dbf35cbffa3aef28f8c01d8cf2ffc51ef62b26a       */
/*         f9bda5a68e5ed8c972b17bab0f42e24b19daa7bd408305b1f7bd6c7208c1       */
/*         0e36230e913039b3046dd5fd0ba706a624d33dbaa4d6aab02c82fe09f561       */
/*         01b0fd977b0051f0b0ce0c69f7db857b1b5e007be2db6d42894bf93de848       */
/*         806d9152bd5715e9                                                   */
/* Please understand that you are NOT permitted to distribute or publically   */
/*         display a copy of this file (or ANY PART of it) for any reason.    */
/* If anyone (including your prospective employer) asks you to post the code, */
/*         you must inform them that you do NOT have permissions to do so.    */
/* You are also NOT permitted to remove or alter this comment block.          */
/* If this comment block is removed or altered in a submitted file, 20 points */
/*         will be deducted.                                                  */
/******************************************************************************/

#include "globals.h"
#include "errno.h"
#include "types.h"

#include "mm/mm.h"
#include "mm/tlb.h"
#include "mm/mman.h"
#include "mm/page.h"

#include "proc/proc.h"

#include "util/string.h"
#include "util/debug.h"

#include "fs/vnode.h"
#include "fs/vfs.h"
#include "fs/file.h"
#include "fs/stat.h"

#include "vm/vmmap.h"
#include "vm/mmap.h"

/*
 * This function implements the mmap(2) syscall, but only
 * supports the MAP_SHARED, MAP_PRIVATE, MAP_FIXED, and
 * MAP_ANON flags.
 *
 * Add a mapping to the current process's address space.
 * You need to do some error checking; see the ERRORS section
 * of the manpage for the problems you should anticipate.
 * After error checking most of the work of this function is
 * done by vmmap_map(), but remember to clear the TLB.
 */
int
do_mmap(void *addr, size_t len, int prot, int flags,
        int fd, off_t off, void **ret)
{
        //NOT_YET_IMPLEMENTED("VM: do_mmap");
		dbg(DBG_TEMP, "do_mmap entry\n");
        if((fd > MAX_FILES && fd < 0))
        {
        	if(!(MAP_ANON & flags))
        	{
        		dbg(DBG_TEMP, "do_mmap exit\n");
        		dbg(DBG_PRINT, "(GRADING3B X)\n");
        		return -EBADF;	
        	}
        	dbg(DBG_PRINT, "(GRADING3B X)\n");
        }
        if (addr == NULL)
        {
        	if (flags & MAP_ANON)
        	{
        		vmarea_t *area;
        		int newvm = vmmap_map(curproc->p_vmmap, NULL, ADDR_TO_PN(USER_MEM_LOW), ADDR_TO_PN(len),prot, flags, 0, VMMAP_DIR_LOHI, &area);
				*ret = PN_TO_ADDR(area->vma_start);
		        //fput(new_f);
		        tlb_flush_range((uintptr_t)addr,(uint32_t)len);
		        dbg(DBG_TEMP, "do_mmap exit\n");
		        KASSERT(NULL != curproc->p_pagedir);
            	dbg(DBG_PRINT, "(GRADING3A 2.a)\n");
		        return newvm;
        	}
        	file_t *new_file = fget(fd);
        	if(new_file == NULL)
	        {
	        	return -EBADF;
	        }
	        if (new_file->f_vnode == NULL)
        	{
        		fput(new_file);
        		return -ENODEV;
        	}
        	vnode_t *new_node = new_file->f_vnode;
        	
        	vref(new_node);
        	vmarea_t *new_area;
        	int newres = vmmap_map(curproc->p_vmmap, new_node, ADDR_TO_PN(USER_MEM_LOW) ,(uint32_t)len,prot,flags, off, VMMAP_DIR_LOHI, &new_area);
        	fput(new_file);
        	*ret = PN_TO_ADDR(new_area->vma_start);
        	
	        tlb_flush_range((USER_MEM_LOW),(uint32_t)len);
	        dbg(DBG_TEMP, "do_mmap exit\n");
	        KASSERT(NULL != curproc->p_pagedir);
            dbg(DBG_PRINT, "(GRADING3A 2.a)\n");

	        dbg(DBG_PRINT, "(GRADING3C 1)\n");
	        return newres;
        }
        if((uint32_t)addr > USER_MEM_HIGH || (uint32_t)addr < USER_MEM_LOW || len <=0)
        {
        	dbg(DBG_TEMP, "do_mmap exit\n");
        	dbg(DBG_PRINT, "(GRADING3B X)\n");
        	return -EINVAL;
        }
        if(!PAGE_ALIGNED(addr) || !PAGE_ALIGNED(off) || !PAGE_ALIGNED(len))
        {
        	dbg(DBG_TEMP, "do_mmap exit\n");
        	dbg(DBG_PRINT, "(GRADING3B X)\n");
        	return -EINVAL;
        }
        if(!(flags & MAP_PRIVATE) && !((flags & MAP_SHARED)))
        {
        	dbg(DBG_TEMP, "do_mmap exit\n");
        	return -EINVAL;
        }
        if ((flags & MAP_PRIVATE) && (flags & MAP_SHARED))
        {
        	dbg(DBG_TEMP, "do_mmap exit\n");
        	dbg(DBG_PRINT, "(GRADING3B X)\n");
        	return -EINVAL;
        }
        
        file_t *new_f = fget(fd);
        if(new_f == NULL)
        {
        	dbg(DBG_TEMP, "do_mmap exit\n");
        	dbg(DBG_PRINT, "(GRADING3B X)\n");
        	return -EBADF;
        }
        if ((prot & PROT_WRITE) && (new_f->f_mode & FMODE_APPEND))
        {
        	fput(new_f);
        	dbg(DBG_TEMP, "do_mmap exit\n");
        	dbg(DBG_PRINT, "(GRADING3B X)\n");
        	return -EACCES;
        }

			if((flags & MAP_PRIVATE) && !(new_f->f_mode & FMODE_READ))
			{
				dbg(DBG_TEMP, "do_mmap exit\n");
        		return -EACCES;
			}

			if ((flags & MAP_SHARED) && (prot & PROT_WRITE) && (!(new_f->f_mode & FMODE_READ) || !(new_f->f_mode & FMODE_WRITE)))
			{
				dbg(DBG_TEMP, "do_mmap exit\n");
				return -EACCES;
			}
        	

			if (!(S_ISREG(new_f->f_vnode->vn_mode) || (S_ISCHR(new_f->f_vnode->vn_mode))))
			{
				dbg(DBG_TEMP, "do_mmap exit\n");
        		return -EACCES;
			}
	
        vnode_t *new_n = new_f->f_vnode;
        vmarea_t *new_a;
        int newvm, res;
        if(flags & MAP_ANON)
		{
			int newvm = vmmap_map(curproc->p_vmmap, NULL, ADDR_TO_PN(addr), ADDR_TO_PN(len),prot, flags, off, VMMAP_DIR_LOHI, &new_a);
			*ret = PN_TO_ADDR(new_a->vma_start);
	        fput(new_f);
	        tlb_flush_range((uintptr_t)addr,(uint32_t)len);
	        dbg(DBG_TEMP, "do_mmap exit\n");
	        KASSERT(NULL != curproc->p_pagedir);
            dbg(DBG_PRINT, "(GRADING3A 2.a)\n");

	        dbg(DBG_PRINT, "(GRADING3B X)\n");
	        return newvm;
		}
		else
		{
			int res = vmmap_map(curproc->p_vmmap, new_n, ADDR_TO_PN(addr), ADDR_TO_PN(len),prot,flags, off, VMMAP_DIR_LOHI, &new_a);
			
			*ret = PN_TO_ADDR(new_a->vma_start);
	        fput(new_f);
	        tlb_flush_range((uintptr_t)addr,(uint32_t)len);
	        dbg(DBG_TEMP, "do_mmap exit\n");
	        KASSERT(NULL != curproc->p_pagedir);
            dbg(DBG_PRINT, "(GRADING3A 2.a)\n");
	        dbg(DBG_PRINT, "(GRADING3B X)\n");
	        return res;
		}
        
        
}


/*
 * This function implements the munmap(2) syscall.
 *
 * As with do_mmap() it should perform the required error checking,
 * before calling upon vmmap_remove() to do most of the work.
 * Remember to clear the TLB.
 */
int
do_munmap(void *addr, size_t len)
{
        //NOT_YET_IMPLEMENTED("VM: do_munmap");
		dbg(DBG_TEMP, "do_munmap exit\n");
		uint32_t page_num =  ADDR_TO_PN(addr);
		uint32_t length = ADDR_TO_PN(len);
		if(PAGE_ALIGNED(len)<=0)
		{
        	len = (uint32_t)PN_TO_ADDR(length +1);
        	dbg(DBG_PRINT, "(GRADING3B X)\n");
		}
		//uint32_t end = (uint32_t)addr +len;
		if(PAGE_ALIGNED(addr)<=0 || len<=0 || ((uint32_t)addr < USER_MEM_LOW && (uint32_t)addr > USER_MEM_HIGH)) 
		{
			dbg(DBG_TEMP, "do_munmap exit\n");
			dbg(DBG_PRINT, "(GRADING3B X)\n");
			return -EINVAL;
		}
		vmmap_remove(curproc->p_vmmap, page_num , length);
		tlb_flush_all();
		dbg(DBG_TEMP, "do_munmap exit\n");
		dbg(DBG_PRINT, "(GRADING3B X)\n");
        return 0;
}

