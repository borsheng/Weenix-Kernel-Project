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
#include "config.h"
#include "globals.h"
#include "errno.h"

#include "util/debug.h"
#include "util/list.h"
#include "util/string.h"
#include "util/printf.h"

#include "proc/kthread.h"
#include "proc/proc.h"
#include "proc/sched.h"
#include "proc/proc.h"

#include "mm/slab.h"
#include "mm/page.h"
#include "mm/mmobj.h"
#include "mm/mm.h"
#include "mm/mman.h"

#include "vm/vmmap.h"

#include "fs/vfs.h"
#include "fs/vfs_syscall.h"
#include "fs/vnode.h"
#include "fs/file.h"

proc_t *curproc = NULL; /* global */
static slab_allocator_t *proc_allocator = NULL;

static list_t _proc_list;
static proc_t *proc_initproc = NULL; /* Pointer to the init process (PID 1) */

int _is_all_threads_exited(list_t *);  /* extra function for checking if all the threads within a process are exited */
proc_t *_dead_proc_lookup(list_t *);   /* extra function for looking up dead process */
void _final_clean_up(proc_t *);             /* extra function to clean up PCB, TCB, TCB's stack, and PCB's page table */

void
proc_init()
{
        list_init(&_proc_list);
        proc_allocator = slab_allocator_create("proc", sizeof(proc_t));
        KASSERT(proc_allocator != NULL);
}

proc_t *
proc_lookup(int pid)
{
        proc_t *p;
        list_iterate_begin(&_proc_list, p, proc_t, p_list_link) {
                if (p->p_pid == pid) {
                        return p;
                }
        } list_iterate_end();
        return NULL;
}

list_t *
proc_list()
{
        return &_proc_list;
}

size_t
proc_info(const void *arg, char *buf, size_t osize)
{
        const proc_t *p = (proc_t *) arg;
        size_t size = osize;
        proc_t *child;

        KASSERT(NULL != p);
        KASSERT(NULL != buf);

        iprintf(&buf, &size, "pid:          %i\n", p->p_pid);
        iprintf(&buf, &size, "name:         %s\n", p->p_comm);
        if (NULL != p->p_pproc) {
                iprintf(&buf, &size, "parent:       %i (%s)\n",
                        p->p_pproc->p_pid, p->p_pproc->p_comm);
        } else {
                iprintf(&buf, &size, "parent:       -\n");
        }

#ifdef __MTP__
        int count = 0;
        kthread_t *kthr;
        list_iterate_begin(&p->p_threads, kthr, kthread_t, kt_plink) {
                ++count;
        } list_iterate_end();
        iprintf(&buf, &size, "thread count: %i\n", count);
#endif

        if (list_empty(&p->p_children)) {
                iprintf(&buf, &size, "children:     -\n");
        } else {
                iprintf(&buf, &size, "children:\n");
        }
        list_iterate_begin(&p->p_children, child, proc_t, p_child_link) {
                iprintf(&buf, &size, "     %i (%s)\n", child->p_pid, child->p_comm);
        } list_iterate_end();

        iprintf(&buf, &size, "status:       %i\n", p->p_status);
        iprintf(&buf, &size, "state:        %i\n", p->p_state);

#ifdef __VFS__
#ifdef __GETCWD__
        if (NULL != p->p_cwd) {
                char cwd[256];
                lookup_dirpath(p->p_cwd, cwd, sizeof(cwd));
                iprintf(&buf, &size, "cwd:          %-s\n", cwd);
        } else {
                iprintf(&buf, &size, "cwd:          -\n");
        }
#endif /* __GETCWD__ */
#endif

#ifdef __VM__
        iprintf(&buf, &size, "start brk:    0x%p\n", p->p_start_brk);
        iprintf(&buf, &size, "brk:          0x%p\n", p->p_brk);
#endif

        return size;
}

size_t
proc_list_info(const void *arg, char *buf, size_t osize)
{
        size_t size = osize;
        proc_t *p;

        KASSERT(NULL == arg);
        KASSERT(NULL != buf);

#if defined(__VFS__) && defined(__GETCWD__)
        iprintf(&buf, &size, "%5s %-13s %-18s %-s\n", "PID", "NAME", "PARENT", "CWD");
#else
        iprintf(&buf, &size, "%5s %-13s %-s\n", "PID", "NAME", "PARENT");
#endif

        list_iterate_begin(&_proc_list, p, proc_t, p_list_link) {
                char parent[64];
                if (NULL != p->p_pproc) {
                        snprintf(parent, sizeof(parent),
                                 "%3i (%s)", p->p_pproc->p_pid, p->p_pproc->p_comm);
                } else {
                        snprintf(parent, sizeof(parent), "  -");
                }

#if defined(__VFS__) && defined(__GETCWD__)
                if (NULL != p->p_cwd) {
                        char cwd[256];
                        lookup_dirpath(p->p_cwd, cwd, sizeof(cwd));
                        iprintf(&buf, &size, " %3i  %-13s %-18s %-s\n",
                                p->p_pid, p->p_comm, parent, cwd);
                } else {
                        iprintf(&buf, &size, " %3i  %-13s %-18s -\n",
                                p->p_pid, p->p_comm, parent);
                }
#else
                iprintf(&buf, &size, " %3i  %-13s %-s\n",
                        p->p_pid, p->p_comm, parent);
#endif
        } list_iterate_end();
        return size;
}

static pid_t next_pid = 0;

/**
 * Returns the next available PID.
 *
 * Note: Where n is the number of running processes, this algorithm is
 * worst case O(n^2). As long as PIDs never wrap around it is O(n).
 *
 * @return the next available PID
 */
static int
_proc_getid()
{
        proc_t *p;
        pid_t pid = next_pid;
        while (1) {
failed:
                list_iterate_begin(&_proc_list, p, proc_t, p_list_link) {
                        if (p->p_pid == pid) {
                                if ((pid = (pid + 1) % PROC_MAX_COUNT) == next_pid) {
                                        return -1;
                                } else {
                                        goto failed;
                                }
                        }
                } list_iterate_end();
                next_pid = (pid + 1) % PROC_MAX_COUNT;
                return pid;
        }
}

/*
 * The new process, although it isn't really running since it has no
 * threads, should be in the PROC_RUNNING state.
 *
 * Don't forget to set proc_initproc when you create the init
 * process. You will need to be able to reference the init process
 * when reparenting processes to the init process.
 */
proc_t *
proc_create(char *name)
{
        // allocate memory for the new process
        proc_t *p = (proc_t *)slab_obj_alloc(proc_allocator);
        memset(p, 0, sizeof(proc_t));

        // assign a unique PID to the process
        p->p_pid = _proc_getid();

        // copy the process name, ensuring not to overflow the buffer
        strncpy(p->p_comm, name, PROC_NAME_LEN);
        p->p_comm[PROC_NAME_LEN - 1] = '\0';

        // initialize the list of threads and children and parent for this process
        list_init(&p->p_threads);
        list_init(&p->p_children);
        p->p_pproc = curproc;

        KASSERT(PID_IDLE != p->p_pid || list_empty(&_proc_list));
        KASSERT(PID_INIT != p->p_pid || PID_IDLE == curproc->p_pid);
        dbg(DBG_PRINT,"(GRADING1A 2.a)\n");

        // initialize process status and state
        p->p_status = 0;
        p->p_state = PROC_RUNNING;

        // initialize scheduling and memory management components
        sched_queue_init(&p->p_wait);
        p->p_pagedir = pt_create_pagedir();

        // handle special case for PID_INIT
        if (p->p_pid == PID_INIT) {
                proc_initproc = p;
                dbg(DBG_PRINT,"(GRADING1A)\n");
        }

        // insert the process into the global process list
        list_insert_tail(&_proc_list, &p->p_list_link);

        if (curproc) {
                list_insert_tail(&curproc->p_children, &p->p_child_link);
                dbg(DBG_PRINT,"(GRADING1A)\n");
        }
        
        dbg(DBG_PRINT,"(GRADING1A)\n");

#ifdef __VFS__
        if(p->p_pproc != NULL && p->p_pproc->p_cwd != NULL){
                p->p_cwd = p->p_pproc->p_cwd;
                vref(p->p_cwd);
        }
#endif
        return p;
}

/**
 * Cleans up as much as the process as can be done from within the
 * process. This involves:
 *    - Closing all open files (VFS)
 *    - Cleaning up VM mappings (VM)
 *    - Waking up its parent if it is waiting
 *    - Reparenting any children to the init process
 *    - Setting its status and state appropriately
 *
 * The parent will finish destroying the process within do_waitpid (make
 * sure you understand why it cannot be done here). Until the parent
 * finishes destroying it, the process is informally called a 'zombie'
 * process.
 *
 * This is also where any children of the current process should be
 * reparented to the init process (unless, of course, the current
 * process is the init process. However, the init process should not
 * have any children at the time it exits).
 *
 * Note: You do _NOT_ have to special case the idle process. It should
 * never exit this way.
 *
 * @param status the status to exit the process with
 */
void
proc_cleanup(int status)
{
        KASSERT(NULL != proc_initproc);
        dbg(DBG_PRINT, "(GRADING1A 2.b)\n");
        KASSERT(1 <= curproc->p_pid);
        dbg(DBG_PRINT, "(GRADING1A 2.b)\n");
        KASSERT(NULL != curproc->p_pproc);
        dbg(DBG_PRINT, "(GRADING1A 2.b)\n");

        proc_t *child;

        // wake up the parent if it is waiting
        if (!sched_queue_empty(&curproc->p_pproc->p_wait)) {
                sched_wakeup_on(&curproc->p_pproc->p_wait);
                dbg(DBG_PRINT, "(GRADING1A)\n");
        }

        // reparent any children to the init process
        list_iterate_begin(&curproc->p_children, child, proc_t, p_child_link) {
                child->p_pproc = proc_initproc;
                list_remove(&child->p_child_link);
                list_insert_tail(&proc_initproc->p_children, &child->p_child_link);
                dbg(DBG_PRINT,"(GRADING1C)\n");
        } list_iterate_end();

        // set the process state and status
        curproc->p_state = PROC_DEAD;
        curproc->p_status = status;

        KASSERT(NULL != curproc->p_pproc);
        dbg(DBG_PRINT, "(GRADING1A 2.b)\n");
        KASSERT(KT_EXITED == curthr->kt_state);
        dbg(DBG_PRINT, "(GRADING1A 2.b)\n");

        dbg(DBG_PRINT, "(GRADING1A)\n");

#ifdef __VFS__
        vput(curproc->p_cwd);
#endif
}

/*
 * This has nothing to do with signals and kill(1).
 *
 * Calling this on the current process is equivalent to calling
 * do_exit().
 *
 * In Weenix, this is only called from proc_kill_all.
 */
void
proc_kill(proc_t *p, int status)
{
        if (p == curproc) {
               dbg(DBG_PRINT, "(GRADING1C)\n");
               do_exit(status); 
        } else {
                kthread_t *child;
                list_iterate_begin(&p->p_threads, child, kthread_t, kt_plink) {
                        kthread_cancel(child, (void *)status);
                        dbg(DBG_PRINT, "(GRADING1C)\n");
                } list_iterate_end();
                dbg(DBG_PRINT, "(GRADING1C)\n");
        }
}

/*
 * Remember, proc_kill on the current process will _NOT_ return.
 * Don't kill direct children of the idle process.
 *
 * In Weenix, this is only called by sys_halt.
 */
void
proc_kill_all()
{
        proc_t *p;

        // iterate ove the process list to kill each process except the current and idle process
        list_iterate_begin(&_proc_list, p, proc_t, p_list_link) {
                if (p != curproc && p->p_pid != PID_IDLE && p->p_pproc->p_pid != PID_IDLE) {
                        dbg(DBG_PRINT,"(GRADING1C)\n");
                        proc_kill(p, 0);
                }
        } list_iterate_end();

        // handle the current process last to ensure all other processes terminate first
        if (curproc->p_pid != PID_IDLE) {
                dbg(DBG_PRINT,"(GRADING1C)\n");
                proc_kill(curproc, curproc->p_status);
        }
        dbg(DBG_PRINT,"(GRADING1C)\n");
}

/*
 * This function is only called from kthread_exit.
 *
 * Unless you are implementing MTP, this just means that the process
 * needs to be cleaned up and a new thread needs to be scheduled to
 * run. If you are implementing MTP, a single thread exiting does not
 * necessarily mean that the process should be exited.
 */
void
proc_thread_exited(void *retval)
{        // process cleanup using the provided return value
        proc_cleanup((int)(intptr_t)retval);
        dbg(DBG_PRINT, "(GRADING1C)\n");
        sched_switch();
}
        

/* If pid is -1 dispose of one of the exited children of the current
 * process and return its exit status in the status argument, or if
 * all children of this process are still running, then this function
 * blocks on its own p_wait queue until one exits.
 *
 * If pid is greater than 0 and the given pid is a child of the
 * current process then wait for the given pid to exit and dispose
 * of it.
 *
 * If the current process has no children, or the given pid is not
 * a child of the current process return -ECHILD.
 *
 * Pids other than -1 and positive numbers are not supported.
 * Options other than 0 are not supported.
 */
pid_t
do_waitpid(pid_t pid, int options, int *status)
{
        // NOT_YET_IMPLEMENTED("PROCS: do_waitpid");

        //handle invalid pid and options
        dbg(DBG_PRINT, "(GRADING1A)\n");
        if(pid < -1 || options != 0){
                //not supported, print out anything?
                dbg(DBG_PRINT, "(GRADING1A)\n");
                return 0;
        }

        list_t child_procs = curproc->p_children;
        if(list_empty(&child_procs)){
                dbg(DBG_PRINT, "(GRADING1A)\n");
                return -ECHILD;
        }
        
        //returns only when the child process being called upon is PROC_DEAD(set in proc_clean when all its threads are exited)
        proc_t *p = NULL; //proc_waited
        pid_t r_pid = NULL;
        // wait for any child process to die
        dbg(DBG_PRINT, "(GRADING1A)\n");
        if (pid == -1){                
                //none of the child procs is exited, either their threads sleeps on run wait queue or waits on run queue
                dbg(DBG_PRINT, "(GRADING1A)\n");
                while(p == NULL){
                        if(list_empty(&curproc->p_children)){
                                dbg(DBG_PRINT, "(GRADING1C)\n");
                                return -ECHILD;
                        }       
                        // p = _dead_proc_lookup(&child_procs);
                        proc_t *p_dead;
                        //list_iterate_begin(&child_procs, p_dead, proc_t, p_child_link) { //why this doesn't work?
                        dbg(DBG_PRINT, "(GRADING1A)\n");
                        list_iterate_begin(&curproc->p_children, p_dead, proc_t, p_child_link) {
                                dbg(DBG_PRINT, "(GRADING1A)\n");
                                if (p_dead->p_state == PROC_DEAD) {
                                        p = p_dead;
                                        dbg(DBG_PRINT, "(GRADING1A)\n");
                                        break;
                                }
                        } list_iterate_end();
                        dbg(DBG_PRINT, "(GRADING1A)\n");
                        if(p == NULL){
                                /* 
                                     when a proc is cleaned up to some extend by proc_cleanup, it will
                                     check if its parent proc is sleeping in wait queue and will wake 
                                     it up if so. The woken-up parent proc will then be added to kt_runq.
                                     When 
                                 */
                                sched_sleep_on(&curproc->p_wait);
                                dbg(DBG_PRINT, "(GRADING1A)\n");
                        }
                }
        }
        //wait for particular child process with process id = pid to die
        else{   
                dbg(DBG_PRINT, "(GRADING1C)\n");
                p = proc_lookup(pid);
                dbg(DBG_PRINT, "(GRADING1C)\n");
                if(p == NULL){
                        dbg(DBG_PRINT, "(GRADING1C)\n");
                        return -ECHILD;
                }
                //if the proc found is not DEAD yet, make curproc sleep
                dbg(DBG_PRINT, "(GRADING1C)\n");
                while(p->p_state != PROC_DEAD){
                        sched_sleep_on(&curproc->p_wait);
                        dbg(DBG_PRINT, "(GRADING1C)\n");
                }

        }
        //cleanup the the rest of PCB/TCB, stack of TCB and page table of PCB, then return dead proc's pid 
        dbg(DBG_PRINT, "(GRADING1A)\n");
        if(status != NULL){
                dbg(DBG_PRINT, "(GRADING1A)\n");
                *status = p->p_status;
        }      
        dbg(DBG_PRINT, "(GRADING1A)\n");
        r_pid = p->p_pid;
        _final_clean_up(p);

        KASSERT(NULL != p); /* must have found a dead child process */
        dbg(DBG_PRINT, "(GRADING1A 2.c)\n");

        KASSERT(-1 == pid || p->p_pid == pid); /* if the pid argument is not -1, then pid must be the process ID of the found dead child process */
        dbg(DBG_PRINT, "(GRADING1A 2.c)\n");

        KASSERT(NULL != p->p_pagedir); /* this process should have a valid pagedir before you destroy it */
        dbg(DBG_PRINT, "(GRADING1A 2.c)\n");

        return r_pid;
}

/*
 * Cancel all threads and join with them (if supporting MTP), and exit from the current
 * thread.
 *
 * @param status the exit status of the process
 */
void
do_exit(int status)
{
        //NOT_YET_IMPLEMENTED("PROCS: do_exit");
        kthread_t *p;
        list_iterate_begin(&curproc->p_threads, p, kthread_t, kt_plink){
                p->kt_cancelled=1;
                dbg(DBG_PRINT, "(GRADING1C)\n");   
        }list_iterate_end();
        dbg(DBG_PRINT, "(GRADING1C)\n"); 
        kthread_exit((void *)status);

}

// proc_t *
// _dead_proc_lookup(list_t *list){
//         proc_t *p;
//         list_iterate_begin(list, p, proc_t, p_child_link) {
//                 if (p->p_state == PROC_DEAD) {
//                         return p;
//                 }
//         } list_iterate_end();
//         return NULL;
// }

// int
// _is_all_threads_exited(list_t *list){
//         kthread_t *kthr;
//         list_iterate_begin(list, kthr, kthread_t, kt_plink) {
//                 if (kthr->kt_state != KT_EXITED) {
//                         return 0;
//                 }
//         } list_iterate_end();
//         return 1;
// }

void
_final_clean_up(proc_t *p){

        kthread_t *kthr;
        list_t list = p->p_threads;
        /* destroy TCB and its stack */
        //why this doesn't work?
        // list_iterate_begin(&list, kthr, kthread_t, kt_plink) {
        //         kthread_destroy(kthr);
        // } list_iterate_end();
        kthr = list_head(&list, kthread_t, kt_plink);
        kthread_destroy(kthr);

        /* MUST be after kthread_destroy */
        list_remove(&p->p_list_link);    //remove itself from _proc_list
        list_remove(&p->p_child_link);   //remove itself from parent's children list

        /* destroy PCB and its page table */
        pt_destroy_pagedir(p->p_pagedir);
        slab_obj_free(proc_allocator, p);
        dbg(DBG_PRINT, "(GRADING1E)\n");
}