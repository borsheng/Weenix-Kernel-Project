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
#include "kernel.h"
#include "errno.h"

#include "util/gdb.h"
#include "util/init.h"
#include "util/debug.h"
#include "util/string.h"
#include "util/printf.h"

#include "mm/mm.h"
#include "mm/page.h"
#include "mm/pagetable.h"
#include "mm/pframe.h"

#include "vm/vmmap.h"
#include "vm/shadowd.h"
#include "vm/shadow.h"
#include "vm/anon.h"

#include "main/acpi.h"
#include "main/apic.h"
#include "main/interrupt.h"
#include "main/gdt.h"

#include "proc/sched.h"
#include "proc/proc.h"
#include "proc/kthread.h"

#include "drivers/dev.h"
#include "drivers/blockdev.h"
#include "drivers/disk/ata.h"
#include "drivers/tty/virtterm.h"
#include "drivers/pci.h"

#include "api/exec.h"
#include "api/syscall.h"

#include "fs/vfs.h"
#include "fs/vnode.h"
#include "fs/vfs_syscall.h"
#include "fs/fcntl.h"
#include "fs/stat.h"

#include "test/kshell/kshell.h"
#include "test/s5fs_test.h"

GDB_DEFINE_HOOK(initialized)

void      *bootstrap(int arg1, void *arg2);
void      *idleproc_run(int arg1, void *arg2);
kthread_t *initproc_create(void);
void      *initproc_run(int arg1, void *arg2);
void      *final_shutdown(void);

extern void *sunghan_test(int, void*);
extern void *sunghan_deadlock_test(int, void*);
extern void *faber_thread_test(int, void*);


extern int faber_directory_test(kshell_t *ksh, int argc, char **argv);
extern int faber_fs_thread_test(kshell_t *ksh, int argc, char **argv);
extern void *vfstest_main(int, void*);
/**
 * This function is called from kmain, however it is not running in a
 * thread context yet. It should create the idle process which will
 * start executing idleproc_run() in a real thread context.  To start
 * executing in the new process's context call context_make_active(),
 * passing in the appropriate context. This function should _NOT_
 * return.
 *
 * Note: Don't forget to set curproc and curthr appropriately.
 *
 * @param arg1 the first argument (unused)
 * @param arg2 the second argument (unused)
 */
void *
bootstrap(int arg1, void *arg2)
{
        /* If the next line is removed/altered in your submission, 20 points will be deducted. */
        dbgq(DBG_TEST, "SIGNATURE: 53616c7465645f5ff516e73b6598e09d7e7c080d55ebd6c145b76f25053d5664ff3c0e99bdac0219546a0ff1b34511ad\n");
        /* necessary to finalize page table information */
        pt_template_init();

        proc_t *idle_proc = proc_create("IDLE_PROCESS");
        curproc = idle_proc;

        KASSERT(NULL != curproc); /* curproc was uninitialized before, it is initialized here to point to the "idle" process */
        dbg(DBG_PRINT, "(GRADING1A 1.a)\n");

        kthread_t *idle_thr = kthread_create(curproc, idleproc_run, 0, NULL);

        KASSERT(PID_IDLE == curproc->p_pid); /* make sure the process ID of the created "idle" process is PID_IDLE */
        dbg(DBG_PRINT, "(GRADING1A 1.a)\n");

        
        idle_thr-> kt_state = KT_RUN;
        curthr = idle_thr;

        KASSERT(NULL != curthr); /* curthr was uninitialized before, it is initialized here to point to the thread of the "idle" process */
        dbg(DBG_PRINT, "(GRADING1A 1.a)\n");


        context_make_active(&(curthr -> kt_ctx));

        
        dbg(DBG_PRINT, "(GRADING1A)\n");
        panic("weenix returned to bootstrap()!!! BAD!!!\n");
        return NULL;
}

/**
 * Once we're inside of idleproc_run(), we are executing in the context of the
 * first process-- a real context, so we can finally begin running
 * meaningful code.
 *
 * This is the body of process 0. It should initialize all that we didn't
 * already initialize in kmain(), launch the init process (initproc_run),
 * wait for the init process to exit, then halt the machine.
 *
 * @param arg1 the first argument (unused)
 * @param arg2 the second argument (unused)
 */
void *
idleproc_run(int arg1, void *arg2)
{
        int status;
        pid_t child;

        /* create init proc */
        kthread_t *initthr = initproc_create();
        init_call_all();
        GDB_CALL_HOOK(initialized);

        /* Create other kernel threads (in order) */

#ifdef __VFS__
        /* Once you have VFS remember to set the current working directory
         * of the idle and init processes */
        curproc->p_cwd = vfs_root_vn;
        vref(curproc->p_cwd);

        initthr->kt_proc->p_cwd = vfs_root_vn;
        vref(vfs_root_vn);

        /* Here you need to make the null, zero, and tty devices using mknod */
        /* You can't do this until you have VFS, check the include/drivers/dev.h
         * file for macros with the device ID's you will need to pass to mknod */
        do_mkdir("/dev");
        do_mknod("/dev/null", S_IFCHR, MKDEVID(1,0));
        do_mknod("/dev/zero", S_IFCHR, MEM_ZERO_DEVID);
        do_mknod("/dev/tty1", S_IFCHR, MKDEVID(2,1));
        do_mknod("/dev/tty0", S_IFCHR, MKDEVID(2,0));
        
#endif

        /* Finally, enable interrupts (we want to make sure interrupts
         * are enabled AFTER all drivers are initialized) */
        intr_enable();

        /* Run initproc */
        sched_make_runnable(initthr);
        /* Now wait for it */
        child = do_waitpid(-1, 0, &status);
        KASSERT(PID_INIT == child);

        return final_shutdown();
}

/**
 * This function, called by the idle process (within 'idleproc_run'), creates the
 * process commonly refered to as the "init" process, which should have PID 1.
 *
 * The init process should contain a thread which begins execution in
 * initproc_run().
 *
 * @return a pointer to a newly created thread which will execute
 * initproc_run when it begins executing
 */
kthread_t *
initproc_create(void)
{       

        proc_t *p = proc_create("INIT_PROCESS");
        kthread_t *thr = kthread_create(p, initproc_run, 0, NULL);

        KASSERT(NULL != p);
        dbg(DBG_PRINT, "(GRADING1A 1.b)\n");


        KASSERT(PID_INIT == p->p_pid);
        dbg(DBG_PRINT, "(GRADING1A 1.b)\n");

        
        KASSERT(NULL != thr);
        dbg(DBG_PRINT, "(GRADING1A 1.b)\n");

        dbg(DBG_PRINT, "(GRADING1A)\n");
        return thr;
}





int do_my_faber_thread_test(kshell_t *kshell, int argc, char **argv){
    KASSERT(kshell != NULL);
    dbg(DBG_TEMP, "(GRADING1C): do_my_faber_thread_test() is invoked, argc = %d, argv = 0x%08x\n",
            argc, (unsigned int)argv);
    /*
     * Must not call a test function directly.
     * Must create a child process, create a thread in that process and
     *     set the test function as the first procedure of that thread,
     *     then wait for the child process to die.
     */

    proc_t *p = proc_create("my_faber_thread_test");
    struct kthread *t;
    t = kthread_create(p, faber_thread_test, 0, NULL);
    sched_make_runnable(t);
    int rv;
    while (do_waitpid(-1, 0, &rv) != -ECHILD);

    dbg(DBG_PRINT, "(GRADING1A)\n");

    return 0;
}

int do_my_sunghan_test(kshell_t *kshell, int argc, char **argv){
    KASSERT(kshell != NULL);
    dbg(DBG_TEMP, "(GRADING1D 1): do_my_sunghan_test() is invoked, argc = %d, argv = 0x%08x\n",
            argc, (unsigned int)argv);
    /*
     * Must not call a test function directly.
     * Must create a child process, create a thread in that process and
     *     set the test function as the first procedure of that thread,
     *     then wait for the child process to die.
     */
    
    proc_t *p = proc_create("my_sunghan_test");
    struct kthread *t;
    t = kthread_create(p, sunghan_test, 0, NULL);
    sched_make_runnable(t);
    int rv;
    while (do_waitpid(-1, 0, &rv) != -ECHILD);
    dbg(DBG_PRINT, "(GRADING1A)\n");
    return 0;
}

int do_my_sunghan_deadlock_test(kshell_t *kshell, int argc, char **argv){
    KASSERT(kshell != NULL);
    dbg(DBG_TEMP, "(GRADING1D 2): do_my_sunghan_deadlock_test() is invoked, argc = %d, argv = 0x%08x\n",
            argc, (unsigned int)argv);
    /*
     * Must not call a test function directly.
     * Must create a child process, create a thread in that process and
     *     set the test function as the first procedure of that thread,
     *     then wait for the child process to die.
     */
    proc_t *p = proc_create("my_sunghan_deadlock_test");
    struct kthread *t;
    t = kthread_create(p, sunghan_deadlock_test, 0, NULL);
    sched_make_runnable(t);
    int rv;
    while (do_waitpid(-1, 0, &rv) != -ECHILD);
    dbg(DBG_PRINT, "(GRADING1A)\n");

    return 0;
}

int do_my_vfstest(kshell_t *kshell, int argc, char **argv){
    KASSERT(kshell != NULL);
    /*
     * Must not call a test function directly.
     * Must create a child process, create a thread in that process and
     *     set the test function as the first procedure of that thread,
     *     then wait for the child process to die.
     */
    
    proc_t *p = proc_create("my_vfstest");
    struct kthread *t;
    t = kthread_create(p, vfstest_main, 1, NULL);
    sched_make_runnable(t);
    int rv;
    while (do_waitpid(-1, 0, &rv) != -ECHILD);

    return 0;
}



void proc_hello() {
  char *const argvec[] = { "/usr/bin/hello", NULL};
  char *const envvec[] = { NULL };
  kernel_execve("/usr/bin/hello", argvec, envvec);
}

void proc_args() {
  char *const argvec[] = { "/usr/bin/args", "ab" ,"cde" ,"fghi" ,"j", NULL};
  char *const envvec[] = { NULL };
  kernel_execve("/usr/bin/args", argvec, envvec);
}

void proc_uname() {
  do_open("/dev/tty0", O_RDONLY);
  do_open("/dev/tty0", O_WRONLY);
  char *const argvec[] = { "/bin/uname", "-a", NULL};
  char *const envvec[] = { NULL };
  kernel_execve("/bin/uname", argvec, envvec);
}

void proc_readme() {
  do_open("/dev/tty0", O_RDONLY);
  do_open("/dev/tty0", O_WRONLY);
  char *const argvec[] = { "/bin/stat", "/README", NULL};
  char *const envvec[] = { NULL };
  kernel_execve("/bin/stat", argvec, envvec);
}

void proc_stat_usr() {
  do_open("/dev/tty0", O_RDONLY);
  do_open("/dev/tty0", O_WRONLY);
  char *const argvec[] = { "/bin/stat", "/usr", NULL};
  char *const envvec[] = { NULL };
  kernel_execve("/bin/stat", argvec, envvec);
}

void proc_usr_bin() {
  do_open("/dev/tty0", O_RDONLY);
  do_open("/dev/tty0", O_WRONLY);
  char *const argvec[] = { "/bin/ls" "/usr/bin", NULL};
  char *const envvec[] = { NULL };
  kernel_execve("/bin/ls", argvec, envvec);
}

int do_my_hello_test(kshell_t *kshell, int argc, char **argv) {

    KASSERT(kshell != NULL);

    /*
     * Must not call a test function directly.
     * Must create a child process, create a thread in that process and
     *     set the test function as the first procedure of that thread,
     *     then wait for the child process to die.
     */
    proc_t *p = proc_create("do_my_hello_test");
    struct kthread *t;
    t = kthread_create(p, (void *)proc_hello, 0, NULL);
    sched_make_runnable(t);
    int rv;
    while (do_waitpid(-1, 0, &rv) != -ECHILD);
    dbg(DBG_PRINT, "(GRADING1A)\n");

    return 0;
}

int do_my_args_test(kshell_t *kshell, int argc, char **argv) {
    KASSERT(kshell != NULL);

    /*
     * Must not call a test function directly.
     * Must create a child process, create a thread in that process and
     *     set the test function as the first procedure of that thread,
     *     then wait for the child process to die.
     */
    proc_t *p = proc_create("do_my_args_test");
    struct kthread *t;
    t = kthread_create(p, (void *)proc_args, 0, NULL);
    sched_make_runnable(t);
    int rv;
    while (do_waitpid(-1, 0, &rv) != -ECHILD);
    dbg(DBG_PRINT, "(GRADING1A)\n");

    return 0;
}

int do_my_uname_test(kshell_t *kshell, int argc, char **argv) {
    KASSERT(kshell != NULL);

    /*
     * Must not call a test function directly.
     * Must create a child process, create a thread in that process and
     *     set the test function as the first procedure of that thread,
     *     then wait for the child process to die.
     */
    proc_t *p = proc_create("do_my_uname_test");
    struct kthread *t;
    t = kthread_create(p, (void *)proc_uname, 0, NULL);
    sched_make_runnable(t);
    int rv;
    while (do_waitpid(-1, 0, &rv) != -ECHILD);
    dbg(DBG_PRINT, "(GRADING1A)\n");

    return 0;
}

int do_my_readme_test(kshell_t *kshell, int argc, char **argv) {
    KASSERT(kshell != NULL);

    /*
     * Must not call a test function directly.
     * Must create a child process, create a thread in that process and
     *     set the test function as the first procedure of that thread,
     *     then wait for the child process to die.
     */
    proc_t *p = proc_create("do_my_readme_test");
    struct kthread *t;
    t = kthread_create(p, (void *)proc_readme, 0, NULL);
    sched_make_runnable(t);
    int rv;
    while (do_waitpid(-1, 0, &rv) != -ECHILD);
    dbg(DBG_PRINT, "(GRADING1A)\n");

    return 0;
}

int do_my_stat_usr_test(kshell_t *kshell, int argc, char **argv) {
    KASSERT(kshell != NULL);

    /*
     * Must not call a test function directly.
     * Must create a child process, create a thread in that process and
     *     set the test function as the first procedure of that thread,
     *     then wait for the child process to die.
     */
    proc_t *p = proc_create("do_my_stat_usr_test");
    struct kthread *t;
    t = kthread_create(p, (void *)proc_stat_usr, 0, NULL);
    sched_make_runnable(t);
    int rv;
    while (do_waitpid(-1, 0, &rv) != -ECHILD);
    dbg(DBG_PRINT, "(GRADING1A)\n");

    return 0;
}

int do_my_usr_bin_test(kshell_t *kshell, int argc, char **argv) {
    KASSERT(kshell != NULL);

    /*
     * Must not call a test function directly.
     * Must create a child process, create a thread in that process and
     *     set the test function as the first procedure of that thread,
     *     then wait for the child process to die.
     */
    proc_t *p = proc_create("do_my_usr_bin_test");
    struct kthread *t;
    t = kthread_create(p, (void *)proc_usr_bin, 0, NULL);
    sched_make_runnable(t);
    int rv;
    while (do_waitpid(-1, 0, &rv) != -ECHILD);
    dbg(DBG_PRINT, "(GRADING1A)\n");

    return 0;
}

void proc_vfstest() {
    do_open("/dev/tty0", O_RDONLY);
    do_open("/dev/tty0", O_WRONLY);
    char *const argvec[] = { "/usr/bin/vfstest", NULL};
    char *const envvec[] = { NULL };
    kernel_execve("/usr/bin/vfstest", argvec, envvec);
}

int do_my__vfstest(kshell_t *kshell, int argc, char **argv) {
    KASSERT(kshell != NULL);

    /*
     * Must not call a test function directly.
     * Must create a child process, create a thread in that process and
     *     set the test function as the first procedure of that thread,
     *     then wait for the child process to die.
     */
    proc_t *p = proc_create("do_my__vfstest");
    struct kthread *t;
    t = kthread_create(p, (void *)proc_vfstest, 0, NULL);
    sched_make_runnable(t);
    int rv;
    while (do_waitpid(-1, 0, &rv) != -ECHILD);
    dbg(DBG_PRINT, "(GRADING1A)\n");

    return 0;
}


void proc_memtest() {
  char *const argvec[] = { "/usr/bin/memtest", NULL};
  char *const envvec[] = { NULL };
  kernel_execve("/usr/bin/memtest", argvec, envvec);
}
int do_my_memtest(kshell_t *kshell, int argc, char **argv) {
    KASSERT(kshell != NULL);

    /*
     * Must not call a test function directly.
     * Must create a child process, create a thread in that process and
     *     set the test function as the first procedure of that thread,
     *     then wait for the child process to die.
     */
    proc_t *p = proc_create("do_my_memtest");
    struct kthread *t;
    t = kthread_create(p, (void *)proc_memtest, 0, NULL);
    sched_make_runnable(t);
    int rv;
    while (do_waitpid(-1, 0, &rv) != -ECHILD);
    dbg(DBG_PRINT, "(GRADING1A)\n");

    return 0;
}

void proc_eatmem() {
  char *const argvec[] = { "/usr/bin/eatmem", NULL};
  char *const envvec[] = { NULL };
  kernel_execve("/usr/bin/eatmem", argvec, envvec);
}
int do_my_eatmem(kshell_t *kshell, int argc, char **argv) {
    KASSERT(kshell != NULL);

    /*
     * Must not call a test function directly.
     * Must create a child process, create a thread in that process and
     *     set the test function as the first procedure of that thread,
     *     then wait for the child process to die.
     */
    proc_t *p = proc_create("do_my_eatmem");
    struct kthread *t;
    t = kthread_create(p, (void *)proc_eatmem, 0, NULL);
    sched_make_runnable(t);
    int rv;
    while (do_waitpid(-1, 0, &rv) != -ECHILD);
    dbg(DBG_PRINT, "(GRADING1A)\n");

    return 0;
}

void proc_forkbomb() {
  char *const argvec[] = { "/usr/bin/forkbomb", "limit250", NULL};
  char *const envvec[] = { NULL };
  kernel_execve("/usr/bin/forkbomb", argvec, envvec);
}
int do_my_forkbomb(kshell_t *kshell, int argc, char **argv) {
    KASSERT(kshell != NULL);

    /*
     * Must not call a test function directly.
     * Must create a child process, create a thread in that process and
     *     set the test function as the first procedure of that thread,
     *     then wait for the child process to die.
     */
    proc_t *p = proc_create("do_my_forkbomb");
    struct kthread *t;
    t = kthread_create(p, (void *)proc_forkbomb, 0, NULL);
    sched_make_runnable(t);
    int rv;
    while (do_waitpid(-1, 0, &rv) != -ECHILD);
    dbg(DBG_PRINT, "(GRADING1A)\n");

    return 0;
}

void proc_stress() {
  char *const argvec[] = { "/usr/bin/stress", "limit250", NULL};
  char *const envvec[] = { NULL };
  kernel_execve("/usr/bin/stress", argvec, envvec);
}
int do_my_stress(kshell_t *kshell, int argc, char **argv) {
    KASSERT(kshell != NULL);

    /*
     * Must not call a test function directly.
     * Must create a child process, create a thread in that process and
     *     set the test function as the first procedure of that thread,
     *     then wait for the child process to die.
     */
    proc_t *p = proc_create("do_my_stress");
    struct kthread *t;
    t = kthread_create(p, (void *)proc_stress, 0, NULL);
    sched_make_runnable(t);
    int rv;
    while (do_waitpid(-1, 0, &rv) != -ECHILD);
    dbg(DBG_PRINT, "(GRADING1A)\n");

    return 0;
}
/**
 * The init thread's function changes depending on how far along your Weenix is
 * developed. Before VM/FI, you'll probably just want to have this run whatever
 * tests you've written (possibly in a new process). After VM/FI, you'll just
 * exec "/sbin/init".
 *
 * Both arguments are unused.
 *
 * @param arg1 the first argument (unused)
 * @param arg2 the second argument (unused)
 */
void *
initproc_run(int arg1, void *arg2)
{
        //NOT_YET_IMPLEMENTED("PROCS: initproc_run");
        char *const argv[] = { NULL };
        char *const envp[] = { NULL };
        kernel_execve("/sbin/init", argv, envp);
        return NULL;
        
    #ifdef __DRIVERS__

        //kshell_add_command("foo", do_foo, "invoke do_foo() to print a message...");
        kshell_add_command("sunghan", do_my_sunghan_test, "Run sunghan_test().");
        kshell_add_command("deadlock", do_my_sunghan_deadlock_test, "Run sunghan_deadlock_test().");
        kshell_add_command("faber", do_my_faber_thread_test, "Run faber_thread_test().");

    #ifdef __VFS__
        kshell_add_command("vfstest", do_my_vfstest, "Run vfstest_main().");
        kshell_add_command("thrtest", faber_fs_thread_test, "Run faber_fs_thread_test().");
        kshell_add_command("dirtest", faber_directory_test, "Run faber_directory_test().");
    #endif /* __VFS__ */

        kshell_t *kshell = kshell_create(0);
        if (NULL == kshell) panic("init: Couldn't create kernel shell\n");
        while (kshell_execute_next(kshell));
        kshell_destroy(kshell);

    #endif /* __DRIVERS__ */



        return NULL;
}
