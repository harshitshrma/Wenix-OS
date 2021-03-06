
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

int anon_count = 0; /* for debugging/verification purposes */

static slab_allocator_t *anon_allocator;

static void anon_ref(mmobj_t *o);
static void anon_put(mmobj_t *o);
static int  anon_lookuppage(mmobj_t *o, uint32_t pagenum, int forwrite, pframe_t **pf);
static int  anon_fillpage(mmobj_t *o, pframe_t *pf);
static int  anon_dirtypage(mmobj_t *o, pframe_t *pf);
static int  anon_cleanpage(mmobj_t *o, pframe_t *pf);

static mmobj_ops_t anon_mmobj_ops = {
        .ref = anon_ref,
        .put = anon_put,
        .lookuppage = anon_lookuppage,
        .fillpage  = anon_fillpage,
        .dirtypage = anon_dirtypage,
        .cleanpage = anon_cleanpage
};

/*
 * This function is called at boot time to initialize the
 * anonymous page sub system. Currently it only initializes the
 * anon_allocator object.
 */
void
anon_init()
{
        //NOT_YET_IMPLEMENTED("VM: anon_init");
        dbg(DBG_TEMP, "anon_init entry\n");
        anon_allocator = slab_allocator_create("Anon", (size_t) sizeof(mmobj_t));
        KASSERT(anon_allocator);
        dbg(DBG_PRINT, "(GRADING3A 4.a)\n");
        dbg(DBG_TEMP, "anon_init exit\n");
        dbg(DBG_PRINT, "(GRADING3B 1)\n");
}

/*
 * You'll want to use the anon_allocator to allocate the mmobj to
 * return, then initialize it. Take a look in mm/mmobj.h for
 * definitions which can be of use here. Make sure your initial
 * reference count is correct.
 */
mmobj_t *
anon_create()
{
        //NOT_YET_IMPLEMENTED("VM: anon_create");
        dbg(DBG_TEMP, "anon_create entry\n");
        mmobj_t *newobj = (mmobj_t *)slab_obj_alloc(anon_allocator);
        mmobj_init(newobj, &anon_mmobj_ops);
        newobj->mmo_refcount = 1;
        dbg(DBG_TEMP, "anon_create exit\n");
        dbg(DBG_PRINT, "(GRADING3B 1)\n");
        return newobj;
}

/* Implementation of mmobj entry points: */

/*
 * Increment the reference count on the object.
 */
static void
anon_ref(mmobj_t *o)
{
        //NOT_YET_IMPLEMENTED("VM: anon_ref");
        dbg(DBG_TEMP, "anon_ref entry\n");
        KASSERT(o && (0 < o->mmo_refcount) && (&anon_mmobj_ops == o->mmo_ops));
        dbg(DBG_PRINT, "(GRADING3A 4.b)\n");
        o->mmo_refcount++;
        dbg(DBG_TEMP, "anon_ref exit\n");
        dbg(DBG_PRINT, "(GRADING3B 1)\n");
}

/*
 * Decrement the reference count on the object. If, however, the
 * reference count on the object reaches the number of resident
 * pages of the object, we can conclude that the object is no
 * longer in use and, since it is an anonymous object, it will
 * never be used again. You should unpin and uncache all of the
 * object's pages and then free the object itself.
 */
static void
anon_put(mmobj_t *o)
{
        //NOT_YET_IMPLEMENTED("VM: anon_put");
        dbg(DBG_TEMP, "anon_put entry\n");
        KASSERT(o && (0 < o->mmo_refcount) && (&anon_mmobj_ops == o->mmo_ops));
        dbg(DBG_PRINT, "(GRADING3A 4.c)\n");
        if (o->mmo_nrespages == (o->mmo_refcount - 1))
        {
                pframe_t *vp;
                list_iterate_begin(&o->mmo_respages, vp, pframe_t, pf_olink) 
                {
                        while (pframe_is_busy(vp))
                        {
                                sched_sleep_on(&(vp->pf_waitq));
                                dbg(DBG_PRINT, "(GRADING3B X)\n");
                        }
                        if (pframe_is_pinned(vp))
                        {
                                pframe_unpin(vp);
                                dbg(DBG_PRINT, "(GRADING3B X)\n");
                        }
                        //pframe_unpin(vp);
                        pframe_free(vp);
                        dbg(DBG_PRINT, "(GRADING3B X)\n");
                } 
                list_iterate_end();
        }
        
        if(--o->mmo_refcount > 0)
        {
                dbg(DBG_TEMP, "anon_put exit\n");
                dbg(DBG_PRINT, "(GRADING3B 1)\n");
                return;
        }
        slab_obj_free(anon_allocator, o);
        dbg(DBG_TEMP, "anon_put exit\n");
        dbg(DBG_PRINT, "(GRADING3B X)\n");
}

/* Get the corresponding page from the mmobj. No special handling is
 * required. */
static int
anon_lookuppage(mmobj_t *o, uint32_t pagenum, int forwrite, pframe_t **pf)
{
        //NOT_YET_IMPLEMENTED("VM: anon_lookuppage");
        dbg(DBG_TEMP, "anon_lookuppage entry and exit\n");
        dbg(DBG_PRINT, "(GRADING3C 2)\n");
        return pframe_get(o, pagenum, pf);
}

/* The following three functions should not be difficult. */

static int
anon_fillpage(mmobj_t *o, pframe_t *pf)
{
        //NOT_YET_IMPLEMENTED("VM: anon_fillpage");
        dbg(DBG_TEMP, "anon_fillpage entry\n");
        KASSERT(pframe_is_busy(pf));
        KASSERT(!pframe_is_pinned(pf));
        dbg(DBG_PRINT, "(GRADING3A 4.d)\n");

        memset(pf->pf_addr, 0 , PAGE_SIZE);
        dbg(DBG_TEMP, "anon_fillpage exit\n");
        dbg(DBG_PRINT, "(GRADING3B 1)\n");
        return 0;
}

static int
anon_dirtypage(mmobj_t *o, pframe_t *pf)
{
        NOT_YET_IMPLEMENTED("VM: anon_dirtypage");

        return 0;
}

static int
anon_cleanpage(mmobj_t *o, pframe_t *pf)
{
        NOT_YET_IMPLEMENTED("VM: anon_cleanpage");

        return 0;
}
