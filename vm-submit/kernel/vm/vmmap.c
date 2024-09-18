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
		vmmap_t *new_vmmap = (vmmap_t*) slab_obj_alloc(vmmap_allocator);

		new_vmmap->vmm_proc = NULL;
		list_init(&(new_vmmap->vmm_list));
		dbg(DBG_PRINT, "(GRADING3A)\n");
        return new_vmmap;
}

/* Removes all vmareas from the address space and frees the
 * vmmap struct. */
void
vmmap_destroy(vmmap_t *map)
{
        //NOT_YET_IMPLEMENTED("VM: vmmap_destroy");
		KASSERT(NULL != map); /* function argument must not be NULL */
		dbg(DBG_PRINT, "(GRADING3A 3.a)\n");

		vmarea_t *vma; 
		vma = NULL;
		

		while(!list_empty(&map->vmm_list)){

			vma = list_head(&map->vmm_list, vmarea_t, vma_plink);
			if(vma){
				list_remove(&vma->vma_plink);
            	list_remove(&vma->vma_olink);
			}
            
            vma->vma_obj->mmo_ops->put(vma->vma_obj);
            vma->vma_obj = NULL;
            vmarea_free(vma);
			dbg(DBG_PRINT, "(GRADING3A)\n");
		}



		slab_obj_free(vmmap_allocator, map);
		dbg(DBG_PRINT, "(GRADING3A)\n");
}

/* Add a vmarea to an address space. Assumes (i.e. asserts to some extent)
 * the vmarea is valid.  This involves finding where to put it in the list
 * of VM areas, and adding it. Don't forget to set the vma_vmmap for the
 * area. */
void
vmmap_insert(vmmap_t *map, vmarea_t *newvma)
{
        //NOT_YET_IMPLEMENTED("VM: vmmap_insert");
		KASSERT(NULL != map && NULL != newvma); /* both function arguments must not be NULL */
		dbg(DBG_PRINT, "(GRADING3A 3.b)\n");
		KASSERT(NULL == newvma->vma_vmmap); /* newvma must be newly create and must not be part of any existing vmmap */
		dbg(DBG_PRINT, "(GRADING3A 3.b)\n");
		KASSERT(newvma->vma_start < newvma->vma_end); /* newvma must not be empty */
		dbg(DBG_PRINT, "(GRADING3A 3.b)\n");
		KASSERT(ADDR_TO_PN(USER_MEM_LOW) <= newvma->vma_start && ADDR_TO_PN(USER_MEM_HIGH) >= newvma->vma_end);
                             /* addresses in this memory segment must lie completely within the user space */
		dbg(DBG_PRINT, "(GRADING3A 3.b)\n");

		vmarea_t *vma;
		uint32_t cur;
		uint32_t MAX_VFN = ADDR_TO_PN(USER_MEM_HIGH);
		list_iterate_begin(&map->vmm_list, vma, vmarea_t, vma_plink) {

			cur = vma -> vma_start;
			if(newvma->vma_start < vma->vma_start){
				list_insert_before(&vma->vma_plink, &newvma->vma_plink);
                dbg(DBG_PRINT, "(GRADING3A)\n");
                return;
			}

            dbg(DBG_PRINT, "(GRADING3A)\n");   
        }
        list_iterate_end();
        cur = MAX_VFN+1;
        if(cur >= newvma->vma_end){
			list_insert_tail(&map->vmm_list, &newvma->vma_plink);
			dbg(DBG_PRINT, "(GRADING3A)\n");  
		}

        
        dbg(DBG_PRINT, "(GRADING3A)\n");
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
    vmarea_t *vma;
    uint32_t startvfn;
    startvfn = ADDR_TO_PN(USER_MEM_LOW); 
    uint32_t endvfn;
    uint32_t MAX_VFN;
    MAX_VFN = ADDR_TO_PN(USER_MEM_HIGH);
    uint32_t MIN_VFN;
    MIN_VFN = ADDR_TO_PN(USER_MEM_LOW);
    uint32_t cur;

    if(dir == VMMAP_DIR_LOHI){
        endvfn = startvfn + npages;

        list_iterate_begin(&map->vmm_list, vma, vmarea_t, vma_plink) {
            cur = vma->vma_start;
            if (endvfn <= cur){
                
                dbg(DBG_PRINT, "(GRADING3D)\n");
                return startvfn;
            }


            startvfn = vma-> vma_end;
            endvfn = startvfn + npages;

            dbg(DBG_PRINT, "(GRADING3D)\n");
        }

        list_iterate_end();
        MAX_VFN = ADDR_TO_PN(USER_MEM_HIGH);
        if (endvfn <= MAX_VFN){
            dbg(DBG_PRINT, "(GRADING3D)\n");
            return startvfn;
        }
        dbg(DBG_PRINT, "(GRADING3A)\n");

    } else if(dir == VMMAP_DIR_HILO){

        endvfn = ADDR_TO_PN(USER_MEM_HIGH);
        startvfn = endvfn - (int32_t)npages;

        list_iterate_reverse(&map->vmm_list, vma, vmarea_t, vma_plink) {
            cur = vma->vma_end;
            if (startvfn >= cur){
                
                dbg(DBG_PRINT, "(GRADING3D)\n");
                return startvfn;
            }

            endvfn = vma->vma_start;
            startvfn = endvfn - npages;
            dbg(DBG_PRINT, "(GRADING3D)\n");
        } 

        list_iterate_end();
        cur = MIN_VFN;
        if (startvfn >= cur){
            dbg(DBG_PRINT, "(GRADING3D)\n");
            return startvfn;
        }
        dbg(DBG_PRINT, "(GRADING3A)\n");
    }

    dbg(DBG_PRINT, "(GRADING3A)\n");
    return -1;
}

/* Find the vm_area that vfn lies in. Simply scan the address space
 * looking for a vma whose range covers vfn. If the page is unmapped,
 * return NULL. */
vmarea_t *
vmmap_lookup(vmmap_t *map, uint32_t vfn)
{
        //NOT_YET_IMPLEMENTED("VM: vmmap_lookup");
		KASSERT(NULL != map); /* the first function argument must not be NULL */
		dbg(DBG_PRINT, "(GRADING3A 3.c)\n");
        vmarea_t *vma;
        vma = NULL;

        list_iterate_begin(&map->vmm_list, vma, vmarea_t, vma_plink) {
	        if (vfn >= vma -> vma_start){
	        	if(vfn < vma -> vma_end){
	        		dbg(DBG_PRINT, "(GRADING3A)\n");
	        		return vma;
	        	}
	            dbg(DBG_PRINT, "(GRADING3A)\n");
	        }
	        dbg(DBG_PRINT, "(GRADING3A)\n");
        } 
        list_iterate_end();
        dbg(DBG_PRINT, "(GRADING3A)\n");
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
    vmmap_t *newMap;
    newMap = vmmap_create();
    vmarea_t *vma;
    list_iterate_begin(&map->vmm_list, vma, vmarea_t, vma_plink) {
        vmarea_t * newvma;
        newvma = vmarea_alloc();
        newvma->vma_start = vma->vma_start; 
        newvma->vma_end = vma->vma_end;
        vmmap_insert(newMap, newvma);
        dbg(DBG_PRINT, "(GRADING3B)\n");
        newvma->vma_vmmap = newMap;
        newvma->vma_flags = vma->vma_flags;
        newvma->vma_off = vma->vma_off;
        newvma->vma_prot = vma->vma_prot;

        dbg(DBG_PRINT, "(GRADING3B)\n");
    }
    list_iterate_end();
    dbg(DBG_PRINT, "(GRADING3B)\n");
    return newMap;
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
vmmap_map(vmmap_t *map, vnode_t *file, uint32_t lopage, uint32_t npages,int prot, int flags, off_t off, int dir, vmarea_t **new)
{
        KASSERT(NULL != map); /* must not add a memory segment into a non-existing vmmap */
        KASSERT(0 < npages); /* number of pages of this memory segment cannot be 0 */
        KASSERT((MAP_SHARED & flags) || (MAP_PRIVATE & flags)); /* must specify whether the memory segment is shared or private */
        KASSERT((0 == lopage) || (ADDR_TO_PN(USER_MEM_LOW) <= lopage)); /* if lopage is not zero, it must be a user space vpn */
        KASSERT((0 == lopage) || (ADDR_TO_PN(USER_MEM_HIGH) >= (lopage + npages)));
            /* if lopage is not zero, the specified page range must lie completely within the user space */
        KASSERT(PAGE_ALIGNED(off)); /* the off argument must be page aligned */
        dbg(DBG_PRINT, "(GRADING3A 3.d)\n");

        uint32_t VFstr; 
        uint32_t numMid=lopage;        
        if (lopage != 0){            
            VFstr = lopage; 
            int isEmpty=vmmap_is_range_empty(map, lopage, npages);
            if (isEmpty){
                numMid+=lopage;
                dbg(DBG_PRINT, "(GRADING3D)\n");
            }
            else{
                vmmap_remove(map, lopage, npages);
                uint32_t numEnd=lopage +npages;
                pt_unmap_range(curproc->p_pagedir, (uintptr_t)PN_TO_ADDR(lopage), (uintptr_t)PN_TO_ADDR(numEnd));
                dbg(DBG_PRINT, "(GRADING3D)\n");
            }
            dbg(DBG_PRINT, "(GRADING3D)\n");

        }
        else {
            int rang;
            rang = vmmap_find_range(map, npages, dir);
            if (rang != -1){
                numMid+=lopage;
                dbg(DBG_PRINT, "(GRADING3D)\n");
            }
            else{
                numMid+=lopage;
                dbg(DBG_PRINT, "(GRADING3D)\n");
                return -1;
            }
            uint32_t vfn=(uint32_t)rang;
            VFstr = vfn ;
            dbg(DBG_PRINT, "(GRADING3D)\n");

        }

        vmarea_t * VarvmA ;
        VarvmA = vmarea_alloc();
        VarvmA->vma_start = VFstr; 
        numMid+=lopage;
        VarvmA->vma_end = VFstr + npages;
        list_link_init(&(VarvmA->vma_plink));
        list_link_init(&(VarvmA->vma_olink));
        numMid+=lopage;
        vmmap_insert(map, VarvmA);
        
        VarvmA->vma_prot = prot;
        uint32_t parOff=ADDR_TO_PN(off);
        VarvmA->vma_off = parOff;
        void* vmaObj=NULL;

        VarvmA->vma_obj = vmaObj;
        numMid+=lopage;
        VarvmA->vma_flags = flags;
        VarvmA->vma_vmmap = map;
        
        int flagchk=(MAP_PRIVATE & flags);
        int protchk=(PROT_WRITE & prot);
        if (flagchk){
            if(protchk){  
                numMid+=lopage;
                VarvmA->vma_obj = shadow_create() ;
                dbg(DBG_PRINT, "(GRADING3D)\n");
            }
            dbg(DBG_PRINT, "(GRADING3D)\n");
        }

        int filchk= (file != NULL) ;
        if (filchk){
            mmobj_t *FLs = vmaObj;
            file->vn_ops->mmap(file, VarvmA, &FLs);
            int vmachk= (VarvmA->vma_obj == NULL) ;
            if ( vmachk ){
                VarvmA->vma_obj = FLs;
                numMid+=lopage;
                list_insert_head(&(FLs->mmo_un.mmo_vmas), &(VarvmA->vma_olink));
                dbg(DBG_PRINT, "(GRADING3D)\n");
            }
            else {
                VarvmA->vma_obj->mmo_un.mmo_bottom_obj = FLs;
                VarvmA->vma_obj->mmo_shadowed = FLs;
                numMid+=lopage;
                list_insert_head(&(FLs->mmo_un.mmo_vmas), &(VarvmA->vma_olink));
                dbg(DBG_PRINT, "(GRADING3D)\n");
            }
            dbg(DBG_PRINT, "(GRADING3D)\n");
        }
        else {
            int vmachk= (VarvmA->vma_obj != NULL) ;
            mmobj_t *anonOB= anon_create();
            if ( vmachk ){
                VarvmA->vma_obj->mmo_un.mmo_bottom_obj = anonOB;
                VarvmA->vma_obj->mmo_shadowed = anonOB;
                numMid+=lopage;
                list_insert_head(&(anonOB->mmo_un.mmo_vmas), &(VarvmA->vma_olink));
                dbg(DBG_PRINT, "(GRADING3D)\n");
            }
            else {
                VarvmA->vma_obj = anonOB;
                numMid+=lopage;
                list_insert_head(&(anonOB->mmo_un.mmo_vmas), &(VarvmA->vma_olink));
                dbg(DBG_PRINT, "(GRADING3D)\n");
            }
            dbg(DBG_PRINT, "(GRADING3D)\n");
        }

        if (!new){
            numMid+=lopage;
            dbg(DBG_PRINT, "(GRADING3D)\n");
        }
        else{
            numMid+=lopage;
            *new = VarvmA;
            dbg(DBG_PRINT, "(GRADING3D)\n");
        }

        dbg(DBG_PRINT, "(GRADING3A)\n");
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
        
        uint32_t VFstr, VFend;
        VFstr = lopage;
        uint32_t numMid=VFstr;
        VFend = lopage + npages;
        vmarea_t *VarvmA;

    list_iterate_begin(&map->vmm_list, VarvmA, vmarea_t, vma_plink) {
        uint32_t numSrt=VarvmA->vma_start;
        uint32_t numEnd=VarvmA->vma_end;
        if (VFstr > numSrt && VFend < numEnd){
 
                vmarea_t * newVarvmA = vmarea_alloc();
                newVarvmA->vma_start = VarvmA->vma_start;
                numMid+=VFstr;
                newVarvmA->vma_end = VFstr;
                list_link_init(&(newVarvmA->vma_plink));
                list_link_init(&(newVarvmA->vma_olink));
                list_insert_before(&VarvmA->vma_plink, &newVarvmA->vma_plink);

                newVarvmA->vma_vmmap = map;
                numMid+=VFstr;
                newVarvmA->vma_off = VarvmA->vma_off;
                newVarvmA->vma_prot = VarvmA->vma_prot;
                
                newVarvmA->vma_flags = VarvmA->vma_flags;
                numMid+=VFstr;
                newVarvmA->vma_obj = VarvmA->vma_obj;
                newVarvmA->vma_obj->mmo_refcount++;
                
                VarvmA->vma_start = VFend;
                numMid+=VFstr;
                uint32_t curSum=VFend - newVarvmA->vma_start;
                VarvmA->vma_off += curSum;
                numMid+=VFstr;
                
                list_insert_head(&(mmobj_bottom_obj(VarvmA->vma_obj)->mmo_un.mmo_vmas), &(newVarvmA->vma_olink));
                
            
            dbg(DBG_PRINT, "(GRADING3D)\n");
        }
        else if (VFstr > numSrt && VFend >= numEnd && VFstr < numEnd){

                VarvmA->vma_end = VFstr;
                numMid+=VFstr;
            
            dbg(DBG_PRINT, "(GRADING3D)\n");
        }
        else if (VFstr <= numSrt && VFend > numSrt && VFend < numEnd){

                uint32_t curSum=VFend - numSrt;
                VarvmA->vma_off += curSum;
                numMid+=VFstr;
                VarvmA->vma_start = VFend;
            
            dbg(DBG_PRINT, "(GRADING3D)\n");
        }
        else if (VFstr <= numSrt && VFend >= numEnd){

                list_remove(&VarvmA->vma_plink);
                list_remove(&(VarvmA->vma_olink)); 
                numMid+=VFstr;
                VarvmA->vma_obj->mmo_ops->put(VarvmA->vma_obj);
                vmarea_free(VarvmA);
                
            
            dbg(DBG_PRINT, "(GRADING3D)\n");
        }

    } list_iterate_end();
    
    dbg(DBG_PRINT, "(GRADING3D)\n");
    return 0;
}

/*
 * Returns 1 if the given address space has no mappings for the
 * given range, 0 otherwise.
 */
int
vmmap_is_range_empty(vmmap_t *map, uint32_t startvfn, uint32_t npages)
{
        vmarea_t *VarvmA;
        uint32_t endvfn = startvfn + npages;
        KASSERT((startvfn < endvfn) && (ADDR_TO_PN(USER_MEM_LOW) <= startvfn) && (ADDR_TO_PN(USER_MEM_HIGH) >= endvfn));
        dbg(DBG_PRINT, "(GRADING3A 3.e)\n");

        
        list_iterate_begin(&map->vmm_list, VarvmA, vmarea_t, vma_plink) {
            uint32_t numEnd=VarvmA->vma_end-1;
            uint32_t numFn=endvfn-1;
            if (startvfn <= numEnd){
                if(VarvmA->vma_start <= numFn){
                    dbg(DBG_PRINT, "(GRADING3A)\n");
                    return 0;
                }
                dbg(DBG_PRINT, "(GRADING3A)\n");
            }
            dbg(DBG_PRINT, "(GRADING3A)\n");
                
        } list_iterate_end();

        dbg(DBG_PRINT, "(GRADING3A)\n");
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
        uint32_t startvfn = ADDR_TO_PN(vaddr);        //right shif PAGE_SHIFT bits to get pagenumber
        uint32_t start_offset = PAGE_OFFSET(vaddr);   //the starting address to write 
        size_t buf_pos = 0;                           //track the last write position where left off

        vmarea_t *vma = NULL;
        pframe_t *pf = NULL;

        //handle non-page-aligned vaddr(handle only the first partial page)
        if(start_offset > 0){

                vma = vmmap_lookup(map, startvfn);
                //since starvfn might not lies at 1st page of the vmarea, use startvfn - vma->vma_start to locate the page it lies in
                uint32_t pagenum = vma->vma_off + startvfn - vma->vma_start; //offest + (which page of all pages that this mmobj manages)
                int ret = pframe_lookup(vma->vma_obj, pagenum, 0, &pf);
                if(ret < 0){
                        dbg(DBG_PRINT, "(GRADING3A)\n");
                        dbg(DBG_PRINT, "(newpath)\n");
                        return ret;
                }
                //dirty the frame before any modification
                pframe_dirty(pf);

                size_t partial_size = PAGE_SIZE - start_offset;

                //case1: first page is big enough to accomodate the data
                if(partial_size > count){
                        memcpy(buf,(void*) ((uint32_t)(pf->pf_addr) + start_offset), count);
                        dbg(DBG_PRINT, "(GRADING3A)\n");
                        dbg(DBG_PRINT, "(newpath)\n");
                        return 0;
                }

                //case2: first page can only accomodate part of the data
                memcpy(buf, (void*) ((uint32_t)(pf->pf_addr) + start_offset), partial_size);

                //MUST have leftover if arrives here, update buf_pos & startvfn for the rest of code              
                buf_pos += partial_size;
                startvfn++;
                dbg(DBG_PRINT, "(GRADING3A)\n");
                dbg(DBG_PRINT, "(newpath)\n");
        }

        //handle page-alinged vaddr
        while(count > 0){
                
                //it's possible that the vmarea needed is more than one, use different startvfn for each round to get correct vmarea
                vma = vmmap_lookup(map, startvfn);

                //since starvfn might not lies at 1st page of the vmarea, use startvfn - vma->vma_start to locate the page it lies in
                uint32_t pagenum = vma->vma_off + startvfn - vma->vma_start; //offest + (which page of all pages that this mmobj manages)
                int ret = pframe_lookup(vma->vma_obj, pagenum, 0, &pf);
                if(ret < 0){
                        dbg(DBG_PRINT, "(GRADING3A)\n");
                        dbg(DBG_PRINT, "(newpath)\n");
                        return ret;
                }
                //dirty the frame before any modification
                pframe_dirty(pf);

                if(count < PAGE_SIZE){ // all the data can be written into this pageframe
                        //copy the cotent inside buf to pframe's virtual address(user's virtual address)
                        memcpy((void*)((uint32_t)buf + buf_pos), pf->pf_addr, count);
                        dbg(DBG_PRINT, "(GRADING3A)\n");
                        dbg(DBG_PRINT, "(newpath)\n");
                        return 0;
                }
                else{
                        memcpy((void*)((uint32_t)buf + buf_pos), pf->pf_addr, PAGE_SIZE);  
                        count -= PAGE_SIZE;
                        buf_pos += PAGE_SIZE;
                        dbg(DBG_PRINT, "(GRADING3A)\n");
                        dbg(DBG_PRINT, "(newpath)\n");
                }
                //hasn't finished writing all data out yet, increment to next page
                startvfn++;
                dbg(DBG_PRINT, "(GRADING3A)\n");
                dbg(DBG_PRINT, "(newpath)\n");
        }
        dbg(DBG_PRINT, "(GRADING3A)\n");
        dbg(DBG_PRINT, "(newpath)\n");
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
        //NOT_YET_IMPLEMENTED("VM: vmmap_write");
        uint32_t startvfn = ADDR_TO_PN(vaddr);        //right shif PAGE_SHIFT bits to get pagenumber
        uint32_t start_offset = PAGE_OFFSET(vaddr);   //the starting address to write 
        size_t buf_pos = 0;                           //track the last write position where left off

        vmarea_t *vma = NULL;
        pframe_t *pf = NULL;

        //handle non-page-aligned vaddr(handle only the first partial page)
        if(start_offset > 0){

                vma = vmmap_lookup(map, startvfn);
                //since starvfn might not lies at 1st page of the vmarea, use startvfn - vma->vma_start to locate the page it lies in
                uint32_t pagenum = vma->vma_off + startvfn - vma->vma_start; //offest + (which page of all pages that this mmobj manages)
                int ret = pframe_lookup(vma->vma_obj, pagenum, 1, &pf);
                if(ret < 0){
                        dbg(DBG_PRINT, "(GRADING3A)\n");
                        dbg(DBG_PRINT, "(newpath)\n");
                        return ret;
                }
                //dirty the frame before any modification
                pframe_dirty(pf);

                size_t partial_size = PAGE_SIZE - start_offset;

                //case1: first page is big enough to accomodate the data
                if(partial_size > count){
                        memcpy((void*) ((uint32_t)(pf->pf_addr) + start_offset), buf, count);
                        dbg(DBG_PRINT, "(GRADING3A)\n");
                        dbg(DBG_PRINT, "(newpath)\n");
                        return 0;
                }

                //case2: first page can only accomodate part of the data
                memcpy((void*) ((uint32_t)(pf->pf_addr) + start_offset), buf, partial_size);

                //MUST have leftover if arrives here, update buf_pos & startvfn for the rest of code              
                buf_pos += partial_size;
                startvfn++;
                dbg(DBG_PRINT, "(GRADING3A)\n");
                dbg(DBG_PRINT, "(newpath)\n");
        }

        //handle page-alinged vaddr
        while(count > 0){
                
                //it's possible that the vmarea needed is more than one, use different startvfn for each round to get correct vmarea
                vma = vmmap_lookup(map, startvfn);

                //since starvfn might not lies at 1st page of the vmarea, use startvfn - vma->vma_start to locate the page it lies in
                uint32_t pagenum = vma->vma_off + startvfn - vma->vma_start; //offest + (which page of all pages that this mmobj manages)
                int ret = pframe_lookup(vma->vma_obj, pagenum, 1, &pf);
                if(ret < 0){
                        dbg(DBG_PRINT, "(GRADING3A)\n");
                        dbg(DBG_PRINT, "(newpath)\n");
                        return ret;
                }
                //dirty the frame before any modification
                pframe_dirty(pf);

                if(count < PAGE_SIZE){ // all the data can be written into this pageframe
                        //copy the cotent inside buf to pframe's virtual address(user's virtual address)
                        memcpy(pf->pf_addr, (void*)((uint32_t)buf + buf_pos), count);
                        dbg(DBG_PRINT, "(GRADING3A)\n");
                        dbg(DBG_PRINT, "(newpath)\n");
                        return 0;
                }
                else{
                        memcpy(pf->pf_addr, (void*)((uint32_t)buf + buf_pos), PAGE_SIZE);  
                        count -= PAGE_SIZE;
                        buf_pos += PAGE_SIZE;
                        dbg(DBG_PRINT, "(GRADING3A)\n");
                        dbg(DBG_PRINT, "(newpath)\n");
                }
                //hasn't finished writing all data out yet, increment to next page
                startvfn++;
                dbg(DBG_PRINT, "(GRADING3A)\n");
                dbg(DBG_PRINT, "(newpath)\n");
        }
        dbg(DBG_PRINT, "(GRADING3A)\n");
        dbg(DBG_PRINT, "(newpath)\n");
        return 0;
}
