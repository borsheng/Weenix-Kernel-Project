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
        int siz=sizeof(mmobj_t);
        shadow_allocator = slab_allocator_create("shadow",siz);
        KASSERT(shadow_allocator);
        dbg(DBG_PRINT, "(GRADING3A 6.a)\n");
        return;
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
        mmobj_t *mObj ;
        int cunt=1;
        mObj = (mmobj_t*) slab_obj_alloc(shadow_allocator);
        mmobj_init(mObj, &shadow_mmobj_ops);
        mObj->mmo_refcount = cunt;
        dbg(DBG_PRINT, "(GRADING3A)\n");
        return mObj;
}

/* Implementation of mmobj entry points: */

/*
 * Increment the reference count on the object.
 */
static void
shadow_ref(mmobj_t *o)
{
        KASSERT(o && (0 < o->mmo_refcount) && (&shadow_mmobj_ops == o->mmo_ops));
        dbg(DBG_PRINT, "(GRADING3A 6.b)\n");
        int cunt=1;
        o->mmo_refcount+=cunt;
        dbg(DBG_PRINT, "(GRADING3A)\n");
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
        int cuntRef=o->mmo_refcount-1;
        int cuntPag=o->mmo_nrespages;
        int remain=cuntRef-cuntPag;
        int nummid=cuntRef+cuntPag; 
        if(remain != 0) 
        {
            nummid+=1;
            dbg(DBG_PRINT, "(GRADING3A)\n");
        }
        else
        {
            KASSERT(o && (0 < o->mmo_refcount) && (&shadow_mmobj_ops == o->mmo_ops));
            dbg(DBG_PRINT, "(GRADING3A 6.c)\n");
            pframe_t* pfCur;
            nummid+=1;
            list_iterate_begin(&o->mmo_respages, pfCur, pframe_t, pf_olink) {
                        pframe_unpin(pfCur);
                        dbg(DBG_PRINT, "(GRADING3A)\n");

                        if(!pframe_is_dirty(pfCur) ) 
                        {
                            nummid+=1;
                            dbg(DBG_PRINT, "(GRADING3A)\n");
                        }
                        else
                        {
                            pframe_clean(pfCur);
                            dbg(DBG_PRINT, "(GRADING3A)\n");
                        }
                        
                        nummid+=1;
                        pframe_free(pfCur);
                        dbg(DBG_PRINT, "(GRADING3A)\n");
            } list_iterate_end();
            dbg(DBG_PRINT, "(GRADING3A)\n");
        }


        void* shd=NULL;
        if (--(o->mmo_refcount)<=0)
        {
            nummid+=1;
            dbg(DBG_PRINT, "(GRADING3A)\n");
        }
        else
        {
            dbg(DBG_PRINT, "(GRADING3A)\n");
            return;
        }

        if (o->mmo_shadowed == shd)
        {
            nummid+=1;
            dbg(DBG_PRINT, "(GRADING3A)\n");
        }
        else
        {
            o->mmo_shadowed->mmo_ops->put(o->mmo_shadowed);
            dbg(DBG_PRINT, "(GRADING3A)\n");
        }
        nummid+=1;
        slab_obj_free(shadow_allocator, o);
        dbg(DBG_PRINT, "(GRADING3A)\n");
        return;
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
        int jugd=forwrite;
        int result;
        int nummid=forwrite; 
        if (!jugd)
        {
            pframe_t *pfOne;
            mmobj_t *mmoOne ;
            mmoOne = o;
            void* ptJug=NULL;

            pfOne = pframe_get_resident(mmoOne, pagenum);
            mmoOne = mmoOne->mmo_shadowed; 
            nummid+=1;
            dbg(DBG_PRINT, "(GRADING3A)\n");

            while (pfOne == ptJug && mmoOne != ptJug)
            {
                pfOne = pframe_get_resident(mmoOne, pagenum);
                mmoOne = mmoOne->mmo_shadowed; 
                nummid+=1;
                dbg(DBG_PRINT, "(GRADING3A)\n");
            }

            int arg1=0;
            if (pfOne || mmoOne)
            {
                (*pf) = pfOne;
                nummid+=1;
                dbg(DBG_PRINT, "(GRADING3A)\n");
            }
            else 
            {
                result = pframe_lookup(mmobj_bottom_obj(o), pagenum, arg1, pf);
                nummid+=1;
                dbg(DBG_PRINT, "(GRADING3A)\n");
                if (result >= 0) 
                {
                    nummid+=1;
                    dbg(DBG_PRINT, "(GRADING3D 4)\n");
                }
                else{
                    nummid+=1;
                    dbg(DBG_PRINT, "(GRADING3D 4)\n");
                    return result;
                }
                dbg(DBG_PRINT, "(GRADING3A)\n");
            }
            dbg(DBG_PRINT, "(GRADING3A)\n");
        }
        else 
        {
            nummid+=1;
            pframe_get(o, pagenum, pf); 
            dbg(DBG_PRINT, "(GRADING3A)\n");
        }

        KASSERT(NULL != (*pf)); 
        KASSERT((pagenum == (*pf)->pf_pagenum) && (!pframe_is_busy(*pf)));
        dbg(DBG_PRINT, "(GRADING3A 6.d)\n");

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
        KASSERT(pframe_is_busy(pf)); 
        KASSERT(!pframe_is_pinned(pf)); 
        dbg(DBG_PRINT, "(GRADING3A 6.d)\n");

        pframe_pin(pf);
        void* ptJug=NULL;
        pframe_t *pfOne;
        pfOne = ptJug;
        int nummid=shadow_count; 
        mmobj_t *mmpOne;
        mmpOne = o->mmo_shadowed;

        pfOne = pframe_get_resident(mmpOne, pf->pf_pagenum); 
        nummid+=1;
        mmpOne = mmpOne->mmo_shadowed; 
        dbg(DBG_PRINT, "(GRADING3A)\n");
        
        while (pfOne == ptJug && mmpOne != ptJug)
        {
            pfOne = pframe_get_resident(mmpOne, pf->pf_pagenum);
            nummid+=1;
            mmpOne = mmpOne->mmo_shadowed; 
            dbg(DBG_PRINT, "(GRADING3A)\n");
        }
        
        int arg1=0;
        if (pfOne || mmpOne)
        {
            nummid+=1;
            dbg(DBG_PRINT, "(GRADING3A)\n");
        }
        else{
            pframe_lookup(mmobj_bottom_obj(o), pf->pf_pagenum, arg1 , &pfOne);
            nummid+=1;
            dbg(DBG_PRINT, "(GRADING3A)\n");
        }
        
        nummid+=1;
        memcpy(pf->pf_addr, pfOne->pf_addr, PAGE_SIZE);
        dbg(DBG_PRINT, "(GRADING3A)\n");
        return 0;
}

/* These next two functions are not difficult. */

static int
shadow_dirtypage(mmobj_t *o, pframe_t *pf)
{
        int res=0;
        pframe_set_dirty(pf);
        dbg(DBG_PRINT, "(GRADING3A)\n");
        return res;
}

static int
shadow_cleanpage(mmobj_t *o, pframe_t *pf)
{
        int res=0;
        pframe_clear_dirty(pf);
        dbg(DBG_PRINT, "(GRADING3A)\n");
        return res;
}
