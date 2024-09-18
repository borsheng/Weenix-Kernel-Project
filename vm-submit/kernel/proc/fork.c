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
 * entirety of Weenix has been leading up to.
 * Go forth and conquer.
 */
int
do_fork(struct regs *regs)
{
        KASSERT(regs != NULL); /* the function argument must be non-NULL */
        dbg(DBG_PRINT, "(GRADING3A 7.a)\n");
        KASSERT(curproc != NULL); /* the parent process, which is curproc, must be non-NULL */
        dbg(DBG_PRINT, "(GRADING3A 7.a)\n");
        KASSERT(curproc->p_state == PROC_RUNNING); /* the parent process must be in the running state and not in the zombie state */
        dbg(DBG_PRINT, "(GRADING3A 7.a)\n");
        
        int i = 1;
        vmarea_t *vm_area, *temp_vm_area;
        mmobj_t *shadow;
        kthread_t *child_kthread;
        // create a new process that is a copy of the current process
        proc_t *child_proc = proc_create(curproc->p_comm);
        // clone the virtual memory map of the current process for the child
        vmmap_t *new_vmmap = vmmap_clone(curproc->p_vmmap);
        child_proc->p_vmmap = new_vmmap;
        child_proc->p_vmmap->vmm_proc = child_proc;

        // iterate through all vm areas in the current process's vmmap
        list_iterate_begin(&(curproc->p_vmmap->vmm_list), vm_area, vmarea_t, vma_plink) {
                temp_vm_area = vmmap_lookup(child_proc->p_vmmap, vm_area->vma_start);
                // increment ref count
                vm_area->vma_obj->mmo_ops->ref(vm_area->vma_obj);
                
                // if the vm_area is private and writable, create a shadow object
                if ((vm_area->vma_flags & MAP_PRIVATE) && (vm_area->vma_prot & PROT_WRITE)) {
                        shadow = vm_area->vma_obj;

                        vm_area->vma_obj = shadow_create();
                        vm_area->vma_obj->mmo_shadowed = shadow;
                        vm_area->vma_obj->mmo_un.mmo_bottom_obj = shadow->mmo_un.mmo_bottom_obj;
                        dbg(DBG_PRINT, "(GRADING3B)\n");

                        temp_vm_area->vma_obj = shadow_create();
                        temp_vm_area->vma_obj->mmo_shadowed = shadow;
                        temp_vm_area->vma_obj->mmo_un.mmo_bottom_obj = shadow->mmo_un.mmo_bottom_obj;
                        dbg(DBG_PRINT, "(GRADING3B)\n");
                } else {
                        temp_vm_area->vma_obj = vm_area->vma_obj;
                        
                        dbg(DBG_PRINT, "(GRADING3B)\n");
                }

                // insert the vm_area into the list of vm_areas for the shadow object
                list_insert_head(&(mmobj_bottom_obj(vm_area->vma_obj)->mmo_un.mmo_vmas), &(temp_vm_area->vma_olink));
                dbg(DBG_PRINT, "(GRADING3B)\n");
        } list_iterate_end();

        // unmap the user space memory to flush old entries and update the TLB
        pt_unmap_range(curproc->p_pagedir, USER_MEM_LOW, USER_MEM_HIGH);
        tlb_flush_all();
        // set return value for child process
        regs->r_eax = 0;

       // copy file descriptors from the parent to the child 
        while (i < NFILES) {
            child_proc->p_files[i] = curproc->p_files[i];
            if (child_proc->p_files[i]) {
                // increase ref count
                fref(child_proc->p_files[i]);
                dbg(DBG_PRINT, "(GRADING3B)\n");
            }
            i++;
            dbg(DBG_PRINT, "(GRADING3B)\n");
        }

        KASSERT(child_proc->p_state == PROC_RUNNING); /* new child process starts in the running state */
        dbg(DBG_PRINT, "(GRADING3A 7.a)\n");
        KASSERT(child_proc->p_pagedir != NULL); /* new child process must have a valid page table */
        dbg(DBG_PRINT, "(GRADING3A 7.a)\n");

        // clone the current thread for the child process
        child_kthread = kthread_clone(curthr);
        child_kthread->kt_proc = child_proc;
        list_insert_tail(&child_proc->p_threads, &child_kthread->kt_plink);

        KASSERT(child_kthread->kt_kstack != NULL); /* thread in the new child process must have a valid kernel stack */
        dbg(DBG_PRINT, "(GRADING3A 7.a)\n");

        // setup the execution context for the new thread
        child_kthread->kt_ctx.c_pdptr = child_proc->p_pagedir;
        child_kthread->kt_ctx.c_eip = (uint32_t) userland_entry;
        child_kthread->kt_ctx.c_esp = (int)fork_setup_stack(regs, child_kthread->kt_kstack);

        // set up memory boundaries for the new process
        child_proc->p_brk = curproc->p_brk;
        child_proc->p_start_brk = curproc->p_start_brk;

        child_kthread->kt_ctx.c_kstack = (uintptr_t)child_kthread->kt_kstack;
        child_kthread->kt_ctx.c_kstacksz = DEFAULT_STACK_SIZE;

        // make the child thread runnable
        sched_make_runnable(child_kthread);
        dbg(DBG_PRINT, "(GRADING3B)\n");

        return child_proc->p_pid;
}
