
#include "globals.h"
#include "errno.h"

#include "util/string.h"
#include "util/debug.h"

#include "mm/mmobj.h"
#include "mm/pframe.h"
#include "mm/mm.h"
#include "mm/page.h"
#include "mm/slab.h"
#include "mm/tlb.h"

#include "vm/vmmap.h"
#include "vm/shadow.h"
#include "vm/shadowd.h"

#define SHADOW_SINGLETON_THRESHOLD 5

int shadow_count = 0; /* for debugging/verification purposes */
#ifdef __SHADOWD__
/*
 * number of shadow objects with a single parent, that is another shadow
 * object in the shadow objects tree(singletons)
 */
static int shadow_singleton_count = 0;
#endif

static slab_allocator_t *shadow_allocator;

static void shadow_ref(mmobj_t *o);
static void shadow_put(mmobj_t *o);
static int  shadow_lookuppage(mmobj_t *o, uint32_t pagenum, int forwrite, pframe_t **pf);
static int  shadow_fillpage(mmobj_t *o, pframe_t *pf);
static int  shadow_dirtypage(mmobj_t *o, pframe_t *pf);
static int  shadow_cleanpage(mmobj_t *o, pframe_t *pf);

static mmobj_ops_t shadow_mmobj_ops = {
        .ref = shadow_ref,
        .put = shadow_put,
        .lookuppage = shadow_lookuppage,
        .fillpage  = shadow_fillpage,
        .dirtypage = shadow_dirtypage,
        .cleanpage = shadow_cleanpage
};

/*
 * This function is called at boot time to initialize the
 * shadow page sub system. Currently it only initializes the
 * shadow_allocator object.
 */
void
shadow_init()
{
        //NOT_YET_IMPLEMENTED("VM: shadow_init");
        dbg(DBG_TEMP, "shadow_init entry\n");
        shadow_allocator = slab_allocator_create("shadow",sizeof(mmobj_t));
        KASSERT(shadow_allocator);
        dbg(DBG_PRINT, "(GRADING3A 6.a)\n");
        dbg(DBG_TEMP, "shadow_init exit\n");
        dbg(DBG_PRINT, "(GRADING3B 7)\n");
}

/*
 * You'll want to use the shadow_allocator to allocate the mmobj to
 * return, then then initialize it. Take a look in mm/mmobj.h for
 * macros or functions which can be of use here. Make sure your initial
 * reference count is correct.
 */
mmobj_t *
shadow_create()
{
        //NOT_YET_IMPLEMENTED("VM: shadow_create");
        dbg(DBG_TEMP, "shadow_create entry\n");
        mmobj_t *newobj = (mmobj_t *)slab_obj_alloc(shadow_allocator);
        mmobj_init(newobj, &shadow_mmobj_ops);
        newobj->mmo_refcount = 1;
        dbg(DBG_TEMP, "shadow_create exit\n");
        dbg(DBG_PRINT, "(GRADING3B 7)\n");
        return newobj;
}

/* Implementation of mmobj entry points: */

/*
 * Increment the reference count on the object.
 */
static void
shadow_ref(mmobj_t *o)
{
        //NOT_YET_IMPLEMENTED("VM: shadow_ref");
        dbg(DBG_TEMP, "shadow_ref entry\n");
        KASSERT(o && (0 < o->mmo_refcount) && (&shadow_mmobj_ops == o->mmo_ops));
        dbg(DBG_PRINT, "(GRADING3A 6.b)\n");
        o->mmo_refcount++;
        dbg(DBG_TEMP, "shadow_ref exit\n");
        dbg(DBG_PRINT, "(GRADING3B 7)\n");
}

/*
 * Decrement the reference count on the object. If, however, the
 * reference count on the object reaches the number of resident
 * pages of the object, we can conclude that the object is no
 * longer in use and, since it is a shadow object, it will never
 * be used again. You should unpin and uncache all of the object's
 * pages and then free the object itself.
 */
static void
shadow_put(mmobj_t *o)
{
        //NOT_YET_IMPLEMENTED("VM: shadow_put");
        dbg(DBG_TEMP, "shadow_put entry\n");
        KASSERT(o && (0 < o->mmo_refcount) && (&shadow_mmobj_ops == o->mmo_ops));
        dbg(DBG_PRINT, "(GRADING3A 6.c)\n");
        if (o->mmo_nrespages == o->mmo_refcount - 1)
        {
                pframe_t *vp;
                list_iterate_begin(&o->mmo_respages, vp, pframe_t, pf_olink) 
                {
                        if (pframe_is_pinned(vp))
                        {
                                pframe_unpin(vp);
                                dbg(DBG_PRINT, "(GRADING3B X)\n");
                        }
                        pframe_free(vp);
                        dbg(DBG_PRINT, "(GRADING3B 7)\n");
                } 
                list_iterate_end();
                //o->mmo_shadowed->mmo_ops->put(o->mmo_shadowed);
        }     
        if(--o->mmo_refcount > 0)
        {
                dbg(DBG_TEMP, "shadow_put exit\n");
                dbg(DBG_PRINT, "(GRADING3B 7)\n");
                return;
        }  
        slab_obj_free(shadow_allocator, o);
        dbg(DBG_TEMP, "shadow_put exit\n");
        dbg(DBG_PRINT, "(GRADING3B 7)\n");
}

/* This function looks up the given page in this shadow object. The
 * forwrite argument is true if the page is being looked up for
 * writing, false if it is being looked up for reading. This function
 * must handle all do-not-copy-on-not-write magic (i.e. when forwrite
 * is false find the first shadow object in the chain which has the
 * given page resident). copy-on-write magic (necessary when forwrite
 * is true) is handled in shadow_fillpage, not here. It is important to
 * use iteration rather than recursion here as a recursive implementation
 * can overflow the kernel stack when looking down a long shadow chain */
static int
shadow_lookuppage(mmobj_t *o, uint32_t pagenum, int forwrite, pframe_t **pf)
{
        //NOT_YET_IMPLEMENTED("VM: shadow_lookuppage");
        dbg(DBG_TEMP, "shadow_lookuppage entry\n");
        mmobj_t *temp = o->mmo_un.mmo_bottom_obj;
        mmobj_t *sobj = o;
        pframe_t *newpf;
        int n;
        if (forwrite)
        {
                int npf = pframe_get(o, pagenum, pf);
                KASSERT(NULL != (*pf));
                KASSERT((pagenum == (*pf)->pf_pagenum) && (!pframe_is_busy(*pf)));
                dbg(DBG_PRINT, "(GRADING3A 6.d)\n");
                dbg(DBG_TEMP, "shadow_lookuppage exit\n");
                dbg(DBG_PRINT, "(GRADING3B 7)\n");
                return npf;
        }
        else
        {
                newpf = pframe_get_resident(sobj, pagenum);
                while(!newpf && sobj != temp)
                {

                        sobj = sobj->mmo_shadowed;
                        if (sobj != temp)
                        {
                                newpf = pframe_get_resident(sobj, pagenum);
                                dbg(DBG_PRINT, "(GRADING3B 7)\n");
                        }
                        dbg(DBG_PRINT, "(GRADING3B 7)\n");
                        
                }
                if (sobj == temp)
                {
                        if((n = pframe_lookup(sobj, pagenum, forwrite, &newpf)) < 0)
                        {
                                dbg(DBG_TEMP, "shadow_lookuppage exit\n");
                                dbg(DBG_PRINT, "(GRADING3B X)\n");
                                return n;
                        }
                        dbg(DBG_PRINT, "(GRADING3B 7)\n");
                }
               /* if (newpf == NULL)
                {
                        return -EFAULT;
                }*/
        }

        *pf = newpf;
        KASSERT(NULL != (*pf));
        KASSERT((pagenum == (*pf)->pf_pagenum) && (!pframe_is_busy(*pf)));
        dbg(DBG_PRINT, "(GRADING3A 6.d)\n");
        dbg(DBG_TEMP, "shadow_lookuppage exit\n");
        dbg(DBG_PRINT, "(GRADING3B 7)\n");
        return 0;

}

/* As per the specification in mmobj.h, fill the page frame starting
 * at address pf->pf_addr with the contents of the page identified by
 * pf->pf_obj and pf->pf_pagenum. This function handles all
 * copy-on-write magic (i.e. if there is a shadow object which has
 * data for the pf->pf_pagenum-th page then we should take that data,
 * if no such shadow object exists we need to follow the chain of
 * shadow objects all the way to the bottom object and take the data
 * for the pf->pf_pagenum-th page from the last object in the chain).
 * It is important to use iteration rather than recursion here as a
 * recursive implementation can overflow the kernel stack when
 * looking down a long shadow chain */
static int
shadow_fillpage(mmobj_t *o, pframe_t *pf)
{
        //NOT_YET_IMPLEMENTED("VM: shadow_fillpage");
        dbg(DBG_TEMP, "shadow_fillpage entry\n");
        KASSERT(pframe_is_busy(pf));
        KASSERT(!pframe_is_pinned(pf));
        dbg(DBG_PRINT, "(GRADING3A 6.e)\n");
        mmobj_t *temp = o->mmo_shadowed;
        pframe_t *newpf; 
        while(temp->mmo_shadowed != 0)
        {
                list_iterate_begin(&temp->mmo_respages, newpf, pframe_t, pf_olink)
                {
                        if(newpf->pf_pagenum != pf->pf_pagenum)
                        {
                                dbg(DBG_PRINT, "(GRADING3C 1)\n");
                                continue;
                        }
                        else
                        {
                                memcpy(pf->pf_addr, newpf->pf_addr, PAGE_SIZE);
                                dbg(DBG_TEMP, "shadow_fillpage exit\n");
                                dbg(DBG_PRINT, "(GRADING3B 7)\n");
                                return 0;
                        }
                        dbg(DBG_PRINT, "(GRADING3B X)\n");
                }
                list_iterate_end();
                temp = temp->mmo_shadowed;
                dbg(DBG_PRINT, "(GRADING3C 1)\n");
        }

        pframe_get(temp, pf->pf_pagenum, &newpf);
        memcpy(pf->pf_addr, newpf->pf_addr, PAGE_SIZE);
        dbg(DBG_TEMP, "shadow_fillpage exit\n");
        dbg(DBG_PRINT, "(GRADING3B 7)\n");
        return 0;
}

/* These next two functions are not difficult. */

static int
shadow_dirtypage(mmobj_t *o, pframe_t *pf)
{
        NOT_YET_IMPLEMENTED("VM: shadow_dirtypage");
        return 0;
}

static int
shadow_cleanpage(mmobj_t *o, pframe_t *pf)
{
        NOT_YET_IMPLEMENTED("VM: shadow_cleanpage");
        return 0;
}
