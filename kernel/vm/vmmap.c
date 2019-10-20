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

#include "kernel.h"
#include "errno.h"
#include "globals.h"

#include "vm/vmmap.h"
#include "vm/shadow.h"
#include "vm/anon.h"

#include "proc/proc.h"

#include "util/debug.h"
#include "util/list.h"
#include "util/string.h"
#include "util/printf.h"

#include "fs/vnode.h"
#include "fs/file.h"
#include "fs/fcntl.h"
#include "fs/vfs_syscall.h"

#include "mm/slab.h"
#include "mm/page.h"
#include "mm/mm.h"
#include "mm/mman.h"
#include "mm/mmobj.h"

static slab_allocator_t *vmmap_allocator;
static slab_allocator_t *vmarea_allocator;


void
vmmap_init(void)
{
        vmmap_allocator = slab_allocator_create("vmmap", sizeof(vmmap_t));
        KASSERT(NULL != vmmap_allocator && "failed to create vmmap allocator!");
        vmarea_allocator = slab_allocator_create("vmarea", sizeof(vmarea_t));
        KASSERT(NULL != vmarea_allocator && "failed to create vmarea allocator!");
}

vmarea_t *
vmarea_alloc(void)
{
        vmarea_t *newvma = (vmarea_t *) slab_obj_alloc(vmarea_allocator);
        if (newvma) {
                newvma->vma_vmmap = NULL;
        }
        return newvma;
}

void
vmarea_free(vmarea_t *vma)
{
        KASSERT(NULL != vma);
        slab_obj_free(vmarea_allocator, vma);
}

/* a debugging routine: dumps the mappings of the given address space. */
size_t
vmmap_mapping_info(const void *vmmap, char *buf, size_t osize)
{
        KASSERT(0 < osize);
        KASSERT(NULL != buf);
        KASSERT(NULL != vmmap);

        vmmap_t *map = (vmmap_t *)vmmap;
        vmarea_t *vma;
        ssize_t size = (ssize_t)osize;

        int len = snprintf(buf, size, "%21s %5s %7s %8s %10s %12s\n",
                           "VADDR RANGE", "PROT", "FLAGS", "MMOBJ", "OFFSET",
                           "VFN RANGE");

        list_iterate_begin(&map->vmm_list, vma, vmarea_t, vma_plink) {
                size -= len;
                buf += len;
                if (0 >= size) {
                        goto end;
                }

                len = snprintf(buf, size,
                               "%#.8x-%#.8x  %c%c%c  %7s 0x%p %#.5x %#.5x-%#.5x\n",
                               vma->vma_start << PAGE_SHIFT,
                               vma->vma_end << PAGE_SHIFT,
                               (vma->vma_prot & PROT_READ ? 'r' : '-'),
                               (vma->vma_prot & PROT_WRITE ? 'w' : '-'),
                               (vma->vma_prot & PROT_EXEC ? 'x' : '-'),
                               (vma->vma_flags & MAP_SHARED ? " SHARED" : "PRIVATE"),
                               vma->vma_obj, vma->vma_off, vma->vma_start, vma->vma_end);
        } list_iterate_end();

end:
        if (size <= 0) {
                size = osize;
                buf[osize - 1] = '\0';
        }
        /*
        KASSERT(0 <= size);
        if (0 == size) {
                size++;
                buf--;
                buf[0] = '\0';
        }
        */
        return osize - size;
}

/* Create a new vmmap, which has no vmareas and does
 * not refer to a process. */
vmmap_t *
vmmap_create(void)
{
        //NOT_YET_IMPLEMENTED("VM: vmmap_create");
        dbg(DBG_TEMP, "vmmap_create entry\n");
        vmmap_t *newmap = (vmmap_t *)slab_obj_alloc(vmmap_allocator);
        if(newmap==NULL)
        {
            dbg(DBG_TEMP, "vmmap_create exit\n");
            dbg(DBG_PRINT, "(GRADING3B X)\n");
            return NULL;
        }

        list_init(&newmap->vmm_list);
        newmap->vmm_proc = NULL;
        dbg(DBG_TEMP, "vmmap_create exit\n");
        dbg(DBG_PRINT, "(GRADING3B 1)\n");
        return newmap;
}

/* Removes all vmareas from the address space and frees the
 * vmmap struct. */
void
vmmap_destroy(vmmap_t *map)
{
        //NOT_YET_IMPLEMENTED("VM: vmmap_destroy");
        dbg(DBG_TEMP, "vmmap_destroy entry\n");
        KASSERT(NULL != map);
        dbg(DBG_PRINT, "(GRADING3A 3.a)\n");
        vmarea_t *vma;
        list_iterate_begin(&map->vmm_list, vma, vmarea_t, vma_plink)
        {
            //vmmap_remove(map, vma->vma_start, vma->vma_end - vma->vma_start);
            list_remove(&vma->vma_plink);
            if(list_link_is_linked(&vma->vma_olink))
                {
                    list_remove(&vma->vma_olink);
                    dbg(DBG_PRINT, "(GRADING3B 1)\n");
                }
            
            vma->vma_obj->mmo_ops->put(vma->vma_obj);
            vmarea_free(vma);
            dbg(DBG_PRINT, "(GRADING3B 1)\n");
        }
        list_iterate_end();
        slab_obj_free(vmmap_allocator, map);
        dbg(DBG_TEMP, "vmmap_destroy exit\n");
        dbg(DBG_PRINT, "(GRADING3B 1)\n");
}

/* Add a vmarea to an address space. Assumes (i.e. asserts to some extent)
 * the vmarea is valid.  This involves finding where to put it in the list
 * of VM areas, and adding it. Don't forget to set the vma_vmmap for the
 * area. */
void
vmmap_insert(vmmap_t *map, vmarea_t *newvma)
{
        //NOT_YET_IMPLEMENTED("VM: vmmap_insert");
        dbg(DBG_TEMP, "vmmap_insert entry\n");
        KASSERT(NULL != map && NULL != newvma);
        KASSERT(NULL == newvma->vma_vmmap);
        KASSERT(newvma->vma_start < newvma->vma_end);
        KASSERT(ADDR_TO_PN(USER_MEM_LOW) <= newvma->vma_start && ADDR_TO_PN(USER_MEM_HIGH) >= newvma->vma_end);
        dbg(DBG_PRINT, "(GRADING3A 3.b)\n");
        vmarea_t *vmarea;
        if(list_empty(&map->vmm_list))
        {
            list_insert_head(&map->vmm_list, &newvma->vma_plink);
            newvma->vma_vmmap = map;
            dbg(DBG_TEMP, "vmmap_insert exit\n");
            dbg(DBG_PRINT, "(GRADING3B 1)\n");
            return;
        }
        list_iterate_begin(&map->vmm_list, vmarea, vmarea_t, vma_plink)
        {
            if (newvma->vma_end <= vmarea->vma_end)
            {
                list_insert_before(&vmarea->vma_plink, &newvma->vma_plink);
                newvma->vma_vmmap = map;
                dbg(DBG_TEMP, "vmmap_insert exit\n");
                dbg(DBG_PRINT, "(GRADING3B 1)\n");
                return;
            }
            dbg(DBG_PRINT, "(GRADING3B 1)\n");
        }
        list_iterate_end();
        list_insert_tail(&map->vmm_list, &newvma->vma_plink);
        newvma->vma_vmmap = map;
        dbg(DBG_TEMP, "vmmap_insert exit\n");
        dbg(DBG_PRINT, "(GRADING3B 1)\n");
}

/* Find a contiguous range of free virtual pages of length npages in
 * the given address space. Returns starting vfn for the range,
 * without altering the map. Returns -1 if no such range exists.
 *
 * Your algorithm should be first fit. If dir is VMMAP_DIR_HILO, you
 * should find a gap as high in the address space as possible; if dir
 * is VMMAP_DIR_LOHI, the gap should be as low as possible. */
int
vmmap_find_range(vmmap_t *map, uint32_t npages, int dir)
{
        //NOT_YET_IMPLEMENTED("VM: vmmap_find_range");
        dbg(DBG_TEMP, "vmmap_find_range entry\n");
        int start;
        vmarea_t *find;
        vmarea_t *prev;
        vmarea_t *next;
        uint32_t lo = USER_MEM_LOW;
        uint32_t hi = USER_MEM_HIGH;
        if(dir == VMMAP_DIR_HILO)
        {
            list_iterate_reverse(&map->vmm_list, find, vmarea_t, vma_plink)
            {
                next = list_item(find->vma_plink.l_next,vmarea_t, vma_plink);
                if(next->vma_start - find->vma_end >= npages)
                {
                    dbg(DBG_TEMP, "vmmap_find_range exit\n");
                    dbg(DBG_PRINT, "(GRADING3B X)\n");
                    return next->vma_start - npages;
                }
                dbg(DBG_PRINT, "(GRADING3B X)\n");
            }
            list_iterate_end(); 

            if(find->vma_start-lo >=npages)
            {
                dbg(DBG_TEMP, "vmmap_find_range exit\n");
                dbg(DBG_PRINT, "(GRADING3B X)\n");
                return find->vma_start - npages;
            }
        } 
        else
        {
            list_iterate_begin(&map->vmm_list, find, vmarea_t, vma_plink)
            {   
               prev = list_item(find->vma_plink.l_prev,vmarea_t, vma_plink);
                
                if(find->vma_start-prev->vma_end >= npages)
                {
                    dbg(DBG_TEMP, "vmmap_find_range exit\n");
                    dbg(DBG_PRINT, "(GRADING3B X)\n");
                    return prev->vma_end;
                }
                dbg(DBG_PRINT, "(GRADING3B X)\n");
            }
            list_iterate_end(); 
            if(hi - find->vma_end >= npages)
            {
                dbg(DBG_TEMP, "vmmap_find_range exit\n");
                dbg(DBG_PRINT, "(GRADING3B X)\n");
                return find->vma_end;
            }
            dbg(DBG_PRINT, "(GRADING3B X)\n");
        }
        return -1;
}

/* Find the vm_area that vfn lies in. Simply scan the address space
 * looking for a vma whose range covers vfn. If the page is unmapped,
 * return NULL. */
vmarea_t *
vmmap_lookup(vmmap_t *map, uint32_t vfn)
{
        //NOT_YET_IMPLEMENTED("VM: vmmap_lookup");
        dbg(DBG_TEMP, "vmmap_lookup entry\n");
        KASSERT(NULL != map);
        dbg(DBG_PRINT, "(GRADING3A 3.c)\n");
        vmarea_t *find;
        list_iterate_begin(&map->vmm_list, find, vmarea_t, vma_plink)
        {   
            if(find->vma_end > vfn && find->vma_start <= vfn)
            {
                dbg(DBG_TEMP, "vmmap_lookup exit\n");
                dbg(DBG_PRINT, "(GRADING3B 1)\n");
                return find;
            }
            dbg(DBG_PRINT, "(GRADING3B 1)\n");
        }
        list_iterate_end();
        dbg(DBG_TEMP, "vmmap_lookup exit\n");
        dbg(DBG_PRINT, "(GRADING3B 2)\n");
        return NULL;
}

/* Allocates a new vmmap containing a new vmarea for each area in the
 * given map. The areas should have no mmobjs set yet. Returns pointer
 * to the new vmmap on success, NULL on failure. This function is
 * called when implementing fork(2). */
vmmap_t *
vmmap_clone(vmmap_t *map)
{
        //NOT_YET_IMPLEMENTED("VM: vmmap_clone");
        dbg(DBG_TEMP, "vmmap_clone entry\n");
        vmmap_t *new = vmmap_create();
        if (new == NULL)
        {
            dbg(DBG_TEMP, "vmmap_clone exit\n");
            dbg(DBG_PRINT, "(GRADING3B X)\n");
            return NULL;
        }
        vmarea_t *iter = NULL;
        list_iterate_begin(&map->vmm_list, iter, vmarea_t, vma_plink)
        {   
            if(iter != NULL)
            {
                vmarea_t *new_area = vmarea_alloc();
                new_area->vma_start = iter->vma_start;
                new_area->vma_end = iter->vma_end;
                new_area->vma_off = iter->vma_off;
                new_area->vma_flags = iter->vma_flags;
                new_area->vma_prot = iter->vma_prot;
                list_init(&new_area->vma_olink);
                list_init(&new_area->vma_plink);
                vmmap_insert(new, new_area);
                dbg(DBG_PRINT, "(GRADING3B X)\n");
                //new_area->vma_vmmap = new;
                
                //new_area->vma_obj = iter->vma_obj;
                //new_area->vma_obj->mmo_ops->ref(new_area->vma_obj);
            }
            dbg(DBG_PRINT, "(GRADING3B X)\n");
        }
        list_iterate_end();
        new->vmm_proc = map->vmm_proc;
        dbg(DBG_TEMP, "vmmap_clone exit\n");
        dbg(DBG_PRINT, "(GRADING3B X)\n");
        return new;
}

/* Insert a mapping into the map starting at lopage for npages pages.
 * If lopage is zero, we will find a range of virtual addresses in the
 * process that is big enough, by using vmmap_find_range with the same
 * dir argument.  If lopage is non-zero and the specified region
 * contains another mapping that mapping should be unmapped.
 *
 * If file is NULL an anon mmobj will be used to create a mapping
 * of 0's.  If file is non-null that vnode's file will be mapped in
 * for the given range.  Use the vnode's mmap operation to get the
 * mmobj for the file; do not assume it is file->vn_obj. Make sure all
 * of the area's fields except for vma_obj have been set before
 * calling mmap.
 *
 * If MAP_PRIVATE is specified set up a shadow object for the mmobj.
 *
 * All of the input to this function should be valid (KASSERT!).
 * See mmap(2) for for description of legal input.
 * Note that off should be page aligned.
 *
 * Be very careful about the order operations are performed in here. Some
 * operation are impossible to undo and should be saved until there
 * is no chance of failure.
 *
 * If 'new' is non-NULL a pointer to the new vmarea_t should be stored in it.
 */
int 
vmmap_map(vmmap_t *map, vnode_t *file, uint32_t lopage, uint32_t npages,
          int prot, int flags, off_t off, int dir, vmarea_t **new)
{
        //NOT_YET_IMPLEMENTED("VM: vmmap_map");
        dbg(DBG_TEMP, "vmmap_map entry\n");
        KASSERT(NULL != map);
        KASSERT(0 < npages);
        KASSERT((MAP_SHARED & flags) || (MAP_PRIVATE & flags));
        KASSERT((0 == lopage) || (ADDR_TO_PN(USER_MEM_LOW) <= lopage));
        KASSERT((0 == lopage) || (ADDR_TO_PN(USER_MEM_HIGH) >= (lopage + npages)));
        KASSERT(PAGE_ALIGNED(off));
        dbg(DBG_PRINT, "(GRADING3A 3.d)\n");
        vmarea_t *vmarea = vmarea_alloc();
        mmobj_t *newobj;
        int flag=0;

        if(vmarea==NULL)
        {
            dbg(DBG_TEMP, "vmmap_map exit\n");
            dbg(DBG_PRINT, "(GRADING3B X)\n");
            return -ENOMEM;
        }

        int start_page = lopage;

        if(lopage == 0)
        {
            start_page = vmmap_find_range(map,npages,dir);
            if(start_page < 0)
            {
                vmarea_free(vmarea);
                dbg(DBG_TEMP, "vmmap_map exit\n");
                dbg(DBG_PRINT, "(GRADING3B X)\n");
                return -ENOMEM;
            }
            dbg(DBG_PRINT, "(GRADING3B X)\n");
        }
        else
        {
            flag=1;
            dbg(DBG_PRINT, "(GRADING3B 1)\n");
        }

        vmarea->vma_start = start_page;
        vmarea->vma_end = start_page + npages;
        vmarea->vma_off = off;
        vmarea->vma_prot = prot;
        vmarea->vma_flags = flags;

        list_link_init(&vmarea->vma_olink);
        list_link_init(&vmarea->vma_plink);

        if(file == NULL)
        {
            newobj = anon_create();
            if(newobj==NULL)
            {
                vmarea_free(vmarea);
                dbg(DBG_TEMP, "vmmap_map exit\n");
                dbg(DBG_PRINT, "(GRADING3B X)\n");
                return -ENOMEM; //not sure which error to return
            }
            if ((flags & MAP_PRIVATE) ==  MAP_PRIVATE)
            {
                vmarea->vma_obj = shadow_create();  
                vmarea->vma_obj->mmo_shadowed = newobj; 
                vmarea->vma_obj->mmo_un.mmo_bottom_obj = newobj;
                //vmarea->vma_obj->mmo_ops->ref(vmarea->vma_obj);
                list_insert_tail(&vmarea->vma_obj->mmo_shadowed->mmo_un.mmo_vmas, &vmarea->vma_olink);
                dbg(DBG_PRINT, "(GRADING3B 1)\n");
            } 
            else
            {
                vmarea->vma_obj = newobj;
                //vmarea->vma_obj->mmo_ops->ref(vmarea->vma_obj);
                list_insert_tail(&vmarea->vma_obj->mmo_un.mmo_vmas, &vmarea->vma_olink);
                dbg(DBG_PRINT, "(GRADING3B X)\n");
            } 
            dbg(DBG_PRINT, "(GRADING3B 1)\n");
        }
        else
        {
            int res = file->vn_ops->mmap(file,vmarea,&newobj);
            
            if (res<0)
            {
                vmarea_free(vmarea);
                dbg(DBG_TEMP, "vmmap_map exit\n");
                dbg(DBG_PRINT, "(GRADING3B X)\n");
                return res;
            }  
            
            if ((flags & MAP_PRIVATE) ==  MAP_PRIVATE)
            {
                vmarea->vma_obj = shadow_create(); 
                vmarea->vma_obj->mmo_shadowed = newobj;  
                vmarea->vma_obj->mmo_un.mmo_bottom_obj = newobj;
                //vmarea->vma_obj->mmo_ops->ref(vmarea->vma_obj);
                list_insert_tail(&vmarea->vma_obj->mmo_shadowed->mmo_un.mmo_vmas, &vmarea->vma_olink);
                dbg(DBG_PRINT, "(GRADING3B 1)\n");
            } 
            else
            {
                vmarea->vma_obj = newobj;
                //vmarea->vma_obj->mmo_ops->ref(vmarea->vma_obj); 
                list_insert_tail(&vmarea->vma_obj->mmo_un.mmo_vmas, &vmarea->vma_olink); 
                dbg(DBG_PRINT, "(GRADING3B X)\n"); 
            }
            vput(file);
            dbg(DBG_PRINT, "(GRADING3B 1)\n");
        }
        //vmarea->vma_obj = newobj;
        if(flag)
        {
            int res = vmmap_remove(map, start_page, npages);
            if(res<0)
            {
                vmarea_free(vmarea);
                dbg(DBG_TEMP, "vmmap_map exit\n");
                dbg(DBG_PRINT, "(GRADING3B X)\n");
                return res;
            }
            dbg(DBG_PRINT, "(GRADING3B 1)\n");
        }
        if (new)
        {
            *new = vmarea;
            dbg(DBG_PRINT, "(GRADING3C 1)\n");
        }
        vmmap_insert(map,vmarea);
        dbg(DBG_TEMP, "vmmap_map exit\n");
        dbg(DBG_PRINT, "(GRADING3B 1)\n");
        return 0;
}

/*
 * We have no guarantee that the region of the address space being
 * unmapped will play nicely with our list of vmareas.
 *
 * You must iterate over each vmarea that is partially or wholly covered
 * by the address range [addr ... addr+len). The vm-area will fall into one
 * of four cases, as illustrated below:
 *
 * key:
 *          [             ]   Existing VM Area
 *        *******             Region to be unmapped
 *
 * Case 1:  [   ******    ]
 * The region to be unmapped lies completely inside the vmarea. We need to
 * split the old vmarea into two vmareas. be sure to increment the
 * reference count to the file associated with the vmarea.
 *
 * Case 2:  [      *******]**
 * The region overlaps the end of the vmarea. Just shorten the length of
 * the mapping.
 *
 * Case 3: *[*****        ]
 * The region overlaps the beginning of the vmarea. Move the beginning of
 * the mapping (remember to update vma_off), and shorten its length.
 *
 * Case 4: *[*************]**
 * The region completely contains the vmarea. Remove the vmarea from the
 * list.
 */
int
vmmap_remove(vmmap_t *map, uint32_t lopage, uint32_t npages)
{
        //NOT_YET_IMPLEMENTED("VM: vmmap_remove");
        dbg(DBG_TEMP, "vmmap_remove entry\n");
        if (vmmap_is_range_empty(map, lopage, npages)) 
        {
            dbg(DBG_TEMP, "vmmap_remove exit\n");
            dbg(DBG_PRINT, "(GRADING3B 1)\n");
            return 0;
        }
        vmarea_t *iter = NULL;
        list_iterate_begin(&map->vmm_list, iter, vmarea_t, vma_plink)
        {
            if (lopage > iter->vma_start && lopage+npages < iter->vma_end)
            {
                //pt_unmap_range(curproc->p_pagedir, (uintptr_t)PN_TO_ADDR(lopage), (uintptr_t)PN_TO_ADDR(lopage+npages + 1));
                vmarea_t *newarea = vmarea_alloc();
                newarea->vma_start = iter->vma_start;
                newarea->vma_end = lopage;
                newarea->vma_off=iter->vma_off;
                newarea->vma_flags = iter->vma_flags;
                newarea->vma_vmmap = iter->vma_vmmap;
                newarea->vma_prot = iter->vma_prot;
                newarea->vma_obj = iter->vma_obj;
                newarea->vma_obj->mmo_ops->ref(newarea->vma_obj);
                list_link_init(&newarea->vma_plink);
                list_insert_before(&iter->vma_plink, &newarea->vma_plink);
                list_link_init(&newarea->vma_olink);
                iter->vma_off = lopage+npages-iter->vma_start + iter->vma_off;
                iter->vma_start = lopage+npages;
                dbg(DBG_PRINT, "(GRADING3B X)\n");
            }
            else if (lopage > iter->vma_start && lopage+npages >= iter->vma_end)
            {
                //pt_unmap_range(curproc->p_pagedir, (uintptr_t)PN_TO_ADDR(lopage), (uintptr_t)PN_TO_ADDR(iter->vma_end+ 1));
                iter->vma_end = lopage;
                dbg(DBG_PRINT, "(GRADING3B 1)\n");
            }
            else if (lopage <= iter->vma_start && lopage+npages < iter->vma_end)
            {
                //pt_unmap_range(curproc->p_pagedir, (uintptr_t)PN_TO_ADDR(iter->vma_start), (uintptr_t)PN_TO_ADDR(lopage+npages + 1));
                iter->vma_off = npages+lopage - iter->vma_start + iter->vma_off;
                iter->vma_start = lopage+npages;
                dbg(DBG_PRINT, "(GRADING3B X)\n");
            }
            else if (lopage <= iter->vma_start && lopage+npages >= iter->vma_end)
            {
                //pt_unmap_range(curproc->p_pagedir, (uintptr_t)PN_TO_ADDR(iter->vma_start), (uintptr_t)PN_TO_ADDR(iter->vma_end + 1));
                list_remove(&iter->vma_plink);
                iter->vma_obj->mmo_ops->put(iter->vma_obj);
                if(list_link_is_linked(&iter->vma_olink))
                {
                    list_remove(&iter->vma_olink);
                    dbg(DBG_PRINT, "(GRADING3B 1)\n");
                }
                vmarea_free(iter);
                dbg(DBG_PRINT, "(GRADING3B 1)\n");
            }
            dbg(DBG_PRINT, "(GRADING3B 1)\n");
        } list_iterate_end();
        dbg(DBG_TEMP, "vmmap_remove exit\n");
        dbg(DBG_PRINT, "(GRADING3B 1)\n");
        return 0;
}

/*
 * Returns 1 if the given address space has no mappings for the
 * given range, 0 otherwise.
 */
int
vmmap_is_range_empty(vmmap_t *map, uint32_t startvfn, uint32_t npages)
{
        //NOT_YET_IMPLEMENTED("VM: vmmap_is_range_empty");
        dbg(DBG_TEMP, "vmmap_is_range_empty entry\n");
        uint32_t endvfn = startvfn + npages;
        KASSERT((startvfn < endvfn) && (ADDR_TO_PN(USER_MEM_LOW) <= startvfn) && (ADDR_TO_PN(USER_MEM_HIGH) >= endvfn));
        dbg(DBG_PRINT, "(GRADING3A 3.e)\n");
        
        vmarea_t *iterator;
        list_iterate_begin(&map->vmm_list, iterator, vmarea_t, vma_plink)
        {
            if(!(iterator->vma_start >= endvfn || iterator->vma_end <= startvfn))
            {
                dbg(DBG_TEMP, "vmmap_is_range_empty exit\n");
                dbg(DBG_PRINT, "(GRADING3B 1)\n");
                return 0;
            }
            dbg(DBG_PRINT, "(GRADING3B 1)\n");
        }
        list_iterate_end();
        dbg(DBG_TEMP, "vmmap_is_range_empty exit\n");
        dbg(DBG_PRINT, "(GRADING3B 1)\n");
        return 1;
}

/* Read into 'buf' from the virtual address space of 'map' starting at
 * 'vaddr' for size 'count'. To do so, you will want to find the vmareas
 * to read from, then find the pframes within those vmareas corresponding
 * to the virtual addresses you want to read, and then read from the
 * physical memory that pframe points to. You should not check permissions
 * of the areas. Assume (KASSERT) that all the areas you are accessing exist.
 * Returns 0 on success, -errno on error.
 */
int
vmmap_read(vmmap_t *map, const void *vaddr, void *buf, size_t count)
{
        //NOT_YET_IMPLEMENTED("VM: vmmap_read");
        dbg(DBG_TEMP, "vmmap_read entry\n");
        vmarea_t *find = NULL;
        pframe_t *result = NULL;
        char *lbuf = (char *) buf;
        uint32_t offset = PAGE_OFFSET((uint32_t)vaddr);
        uint32_t vfn = ADDR_TO_PN((uint32_t)vaddr);
        size_t temp=0;
        while (count > 0) 
        {
            find = vmmap_lookup(map, vfn);
            int error = pframe_lookup(find->vma_obj, find->vma_off - find->vma_start + vfn , 0, &result);
            if (error < 0) 
            {
                dbg(DBG_TEMP, "vmmap_read exit\n");
                dbg(DBG_PRINT, "(GRADING3B X)\n");
                return error;
            }
            if(count < PAGE_SIZE - offset)
            {
                temp = count;
                dbg(DBG_PRINT, "(GRADING3B 1)\n");
            }
            else
            {
                temp = PAGE_SIZE - offset;
                offset = 0;
                dbg(DBG_PRINT, "(GRADING3B 1)\n");
            }
            char *cur = (char *) result->pf_addr + offset;
            memcpy(lbuf, cur, temp);
            lbuf = lbuf + temp;
            count = count - temp;
            dbg(DBG_PRINT, "(GRADING3B 2)\n");
        }
        dbg(DBG_TEMP, "vmmap_read exit\n");
        dbg(DBG_PRINT, "(GRADING3B 1)\n");
        return 0;
}

/* Write from 'buf' into the virtual address space of 'map' starting at
 * 'vaddr' for size 'count'. To do this, you will need to find the correct
 * vmareas to write into, then find the correct pframes within those vmareas,
 * and finally write into the physical addresses that those pframes correspond
 * to. You should not check permissions of the areas you use. Assume (KASSERT)
 * that all the areas you are accessing exist. Remember to dirty pages!
 * Returns 0 on success, -errno on error.
 */


int
vmmap_write(vmmap_t *map, void *vaddr, const void *buf, size_t count)
{
    dbg(DBG_TEMP, "vmmap_write entry\n");
    vmarea_t *find = NULL;
    pframe_t *result = NULL;
    char *lbuf = (char *) buf;
    uint32_t offset = PAGE_OFFSET((uint32_t)vaddr);
    uint32_t vfn = ADDR_TO_PN((uint32_t)vaddr);
    size_t temp=0;
    while (count > 0) 
    {
        find = vmmap_lookup(map, vfn);
        int error = pframe_lookup(find->vma_obj, find->vma_off - find->vma_start + vfn , 1, &result);
        if (error < 0) 
        {
            dbg(DBG_TEMP, "vmmap_write exit\n");
            dbg(DBG_PRINT, "(GRADING3B X)\n");
            return error;
        }
        if(count < PAGE_SIZE - offset)
        {
            temp = count;
            dbg(DBG_PRINT, "(GRADING3B 1)\n");
        }
        else
        {
            temp = PAGE_SIZE - offset;
            offset = 0;
            dbg(DBG_PRINT, "(GRADING3B 2)\n");
        }
        char *cur = (char *) result->pf_addr + offset;
        pframe_dirty(result);
        pframe_set_busy(result);
        memcpy(cur, lbuf, temp);
        pframe_clear_busy(result);
        //sched_broadcast_on(&result->pf_waitq);
        lbuf = lbuf + temp;
        count = count - temp;
        dbg(DBG_PRINT, "(GRADING3B 1)\n");
    }
    dbg(DBG_TEMP, "vmmap_write exit\n");
    dbg(DBG_PRINT, "(GRADING3B 1)\n");
    return 0;
}