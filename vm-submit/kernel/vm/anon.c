/******************************************************************************/
/* Important Spring 2024 CSCI 402 usage information:                          */
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

    anon_allocator = slab_allocator_create("anon",sizeof(mmobj_t));
    KASSERT(anon_allocator);
    dbg(DBG_PRINT, "(GRADING3A 4.a)\n");
    dbg(DBG_PRINT, "(GRADING3A)\n");
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
    mmobj_t *anon_mmobj;
    anon_mmobj = (mmobj_t*) slab_obj_alloc(anon_allocator);
    mmobj_init(anon_mmobj, &anon_mmobj_ops);
    int  tmp = 1;
    anon_mmobj->mmo_refcount = tmp;
    dbg(DBG_PRINT, "(GRADING3A)\n");
    return anon_mmobj;
}

/* Implementation of mmobj entry points: */

/*
 * Increment the reference count on the object.
 */
static void
anon_ref(mmobj_t *o)
{

    KASSERT(o && (0 < o->mmo_refcount) && (&anon_mmobj_ops == o->mmo_ops));
    dbg(DBG_PRINT, "(GRADING3A 4.b)\n");
    o->mmo_refcount++;
    dbg(DBG_PRINT, "(GRADING3A)\n");
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
    
    KASSERT(o && (0 < o->mmo_refcount) && (&anon_mmobj_ops == o->mmo_ops));
    dbg(DBG_PRINT, "(GRADING3A 4.c)\n");
    pframe_t* pf;
    //CAN't decrement mmo_refcount before finishing putting back the page frames, racing happens otherwise
    if((o->mmo_refcount-1) == o->mmo_nrespages) {
        
        list_iterate_begin(&o->mmo_respages, pf, pframe_t, pf_olink) {
                    // check if page frame is busy first, it is where racing happens
                    if(pframe_is_busy(pf)){
                        sched_sleep_on(&pf->pf_waitq);
                        dbg(DBG_PRINT, "(GRADING3D)\n");
                    }

		    
                    

                    // check if the page frames being unpinned is dirty
		    // clean the page frames before freeing them
                    if(pframe_is_dirty(pf) ) {
                        pframe_clean(pf);
                        dbg(DBG_PRINT, "(GRADING3D)\n");
                    }
		    //finally remove the page frame from page table & flush TLB & fill the physical page with MM_POISON_FREE
                    //remove page frames from pinned_list and move them back to alloc_list 
                    pframe_unpin(pf);
                    pframe_free(pf);
                    dbg(DBG_PRINT, "(GRADING3A)\n");

                } list_iterate_end();
        dbg(DBG_PRINT, "(GRADING3A)\n");
    }
    //finally we can decrement refcount
    o->mmo_refcount--;

    //free the slab_obj when refcount is 0
    if (o->mmo_refcount == 0){
        slab_obj_free(anon_allocator, o);
	    dbg(DBG_PRINT, "(GRADING3B)\n");
    }
    dbg(DBG_PRINT, "(GRADING3B)\n");
}

/* Get the corresponding page from the mmobj. No special handling is
 * required. */
static int
anon_lookuppage(mmobj_t *o, uint32_t pagenum, int forwrite, pframe_t **pf)
{
        //NOT_YET_IMPLEMENTED("VM: anon_lookuppage");
		dbg(DBG_PRINT, "(GRADING3A)\n");
    	return pframe_get(o, pagenum, pf);
}

/* The following three functions should not be difficult. */

static int
anon_fillpage(mmobj_t *o, pframe_t *pf)
{
        // NOT_YET_IMPLEMENTED("VM: anon_fillpage");
		KASSERT(pframe_is_busy(pf)); /* can only "fill" a page frame when the page frame is in the "busy" state */
		dbg(DBG_PRINT, "(GRADING3A 4.d)\n");
		KASSERT(!pframe_is_pinned(pf)); /* must not fill a page frame that's already pinned */
		dbg(DBG_PRINT, "(GRADING3A 4.d)\n");
        int zero;
        zero = 0;
		memset(pf->pf_addr, zero, PAGE_SIZE);
		pframe_pin(pf);
		dbg(DBG_PRINT, "(GRADING3A)\n");
        return zero;
}

static int
anon_dirtypage(mmobj_t *o, pframe_t *pf)
{
        int zero;
        zero = 0;
		pframe_set_dirty(pf);
        dbg(DBG_PRINT, "(GRADING3A)\n");
        return zero;
}

static int
anon_cleanpage(mmobj_t *o, pframe_t *pf)
{
        int zero;
        zero = 0;
		pframe_clear_dirty(pf);
        dbg(DBG_PRINT, "(GRADING3A)\n");
        return zero;
}
