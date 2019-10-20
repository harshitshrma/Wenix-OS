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

GDB_DEFINE_HOOK(boot)
GDB_DEFINE_HOOK(initialized)
GDB_DEFINE_HOOK(shutdown)

static void      *bootstrap(int arg1, void *arg2);
static void      *idleproc_run(int arg1, void *arg2);
static kthread_t *initproc_create(void);
static void      *initproc_run(int arg1, void *arg2);
static void       hard_shutdown(void);

extern void *sunghan_test(int, void*);
extern void *sunghan_deadlock_test(int, void*);
extern void *faber_thread_test(int, void*);

extern void *vfstest_main(int, void*);
extern int faber_fs_thread_test(kshell_t *ksh, int argc, char **argv);
extern int faber_directory_test(kshell_t *ksh, int argc, char **argv);

static context_t bootstrap_context;
extern int gdb_wait;

typedef struct {
    struct proc *p;
    struct kthread *t;
} proc_thread_t;
/**
 * This is the first real C function ever called. It performs a lot of
 * hardware-specific initialization, then creates a pseudo-context to
 * execute the bootstrap function in.
 */
void
kmain()
{
        GDB_CALL_HOOK(boot);

        dbg_init();
        dbgq(DBG_CORE, "Kernel binary:\n");
        dbgq(DBG_CORE, "  text: 0x%p-0x%p\n", &kernel_start_text, &kernel_end_text);
        dbgq(DBG_CORE, "  data: 0x%p-0x%p\n", &kernel_start_data, &kernel_end_data);
        dbgq(DBG_CORE, "  bss:  0x%p-0x%p\n", &kernel_start_bss, &kernel_end_bss);

        page_init();

        pt_init();
        slab_init();
        pframe_init();

        acpi_init();
        apic_init();
        pci_init();
        intr_init();

        gdt_init();

        /* initialize slab allocators */
#ifdef __VM__
        anon_init();
        shadow_init();
#endif
        vmmap_init();
        proc_init();
        kthread_init();

#ifdef __DRIVERS__
        bytedev_init();
        blockdev_init();
#endif

        void *bstack = page_alloc();
        pagedir_t *bpdir = pt_get();
        KASSERT(NULL != bstack && "Ran out of memory while booting.");
        /* This little loop gives gdb a place to synch up with weenix.  In the
         * past the weenix command started qemu was started with -S which
         * allowed gdb to connect and start before the boot loader ran, but
         * since then a bug has appeared where breakpoints fail if gdb connects
         * before the boot loader runs.  See
         *
         * https://bugs.launchpad.net/qemu/+bug/526653
         *
         * This loop (along with an additional command in init.gdb setting
         * gdb_wait to 0) sticks weenix at a known place so gdb can join a
         * running weenix, set gdb_wait to zero  and catch the breakpoint in
         * bootstrap below.  See Config.mk for how to set GDBWAIT correctly.
         *
         * DANGER: if GDBWAIT != 0, and gdb is not running, this loop will never
         * exit and weenix will not run.  Make SURE the GDBWAIT is set the way
         * you expect.
         */
        while (gdb_wait) ;
        context_setup(&bootstrap_context, bootstrap, 0, NULL, bstack, PAGE_SIZE, bpdir);
        context_make_active(&bootstrap_context);

        panic("\nReturned to kmain()!!!\n");
}

/**
 * Clears all interrupts and halts, meaning that we will never run
 * again.
 */
static void
hard_shutdown()
{
#ifdef __DRIVERS__
        vt_print_shutdown();
#endif
        __asm__ volatile("cli; hlt");
}

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
static void *
bootstrap(int arg1, void *arg2)
{
        /* If the next line is removed/altered in your submission, 20 points will be deducted. */
        dbgq(DBG_TEST, "SIGNATURE: 53616c7465645f5f3b649a272dfaebbdafaa3df780f9759117a3c9657eb93640e69376d3bf856e1afa45711c318e8330\n");
        /* necessary to finalize page table information */
        pt_template_init();
        proc_thread_t pt;
        pt.p = proc_create("idle");
        pt.t = kthread_create(pt.p, idleproc_run, 0, NULL);
        curproc = pt.p;
        curthr = pt.t;
        curthr->kt_state = KT_RUN;
        KASSERT(NULL != curproc); 
        KASSERT(PID_IDLE == curproc->p_pid);
        KASSERT(NULL != curthr);
        dbg(DBG_PRINT, "(GRADING1A 1.a)\n");
        context_make_active(&curthr->kt_ctx);
        /*NOT_YET_IMPLEMENTED("PROCS: bootstrap");*/

        panic("weenix returned to bootstrap()!!! BAD!!!\n");
        return NULL;
}

#ifdef __DRIVERS__

        int my_faber_thread_test(kshell_t *kshell, int argc, char **argv)
        {
            KASSERT(kshell != NULL);
            dbg(DBG_PRINT, "(GRADING1C): faber_thread_test() is invoked, argc = %d, argv = 0x%08x\n",
                    argc, (unsigned int)argv);
            int rv;
            /*
             * Shouldn't call a test function directly.
             * It's best to invoke it in a separate kernel process.  
             */
            proc_thread_t pt;
            pt.p = proc_create("faber_thread_test");
            pt.t = kthread_create(pt.p, faber_thread_test, argc, argv);
            sched_make_runnable(pt.t);   
            do_waitpid(pt.p->p_pid,0,&rv);
            return 0;
        }

        int my_sunghan_test(kshell_t *kshell, int argc, char **argv)
        {

            KASSERT(kshell != NULL);
            dbg(DBG_PRINT, "(GRADING1D 1): sunghan_test() is invoked, argc = %d, argv = 0x%08x\n",
                    argc, (unsigned int)argv);
            int rv;
            /*
             * Shouldn't call a test function directly.
             * It's best to invoke it in a separate kernel process.  
             */
            proc_thread_t pt;
            pt.p = proc_create("sunghan_test");
            pt.t = kthread_create(pt.p, sunghan_test, argc, argv);
            sched_make_runnable(pt.t);
            do_waitpid(pt.p->p_pid,0,&rv);
            return 0;
        }

        int my_sunghan_deadlock_test(kshell_t *kshell, int argc, char **argv)
        {
            KASSERT(kshell != NULL);
            dbg(DBG_PRINT, "(GRADING1D 2): sunghan_deadlock_test() is invoked, argc = %d, argv = 0x%08x\n",
                    argc, (unsigned int)argv);
            int rv;
            /*
             * Shouldn't call a test function directly.
             * It's best to invoke it in a separate kernel process.  
             */
            proc_thread_t pt;
            pt.p = proc_create("sunghan_deadlock_test");
            pt.t = kthread_create(pt.p, sunghan_deadlock_test, argc, argv);
            sched_make_runnable(pt.t);
            do_waitpid(pt.p->p_pid,0,&rv);
            return 0;
        }

        int my_vfs_test(kshell_t *kshell, int argc, char **argv)
        {
            KASSERT(kshell != NULL);
            dbg(DBG_PRINT, "(GRADING1D 2): vfs_test() is invoked, argc = %d, argv = 0x%08x\n",
                    argc, (unsigned int)argv);
            int rv;
            /*
             * Shouldn't call a test function directly.
             * It's best to invoke it in a separate kernel process.  
             */
            //vfstest_main(argc, argv);
            proc_thread_t pt;
            pt.p = proc_create("vfs_test");
            pt.t = kthread_create(pt.p, vfstest_main, argc, argv);
            sched_make_runnable(pt.t);
            do_waitpid(pt.p->p_pid,0,&rv);
            return 0;
        }
        void test1(int arg1, void *arg2)
        {
            char *const argvec[] = { NULL };
            char *const envvec[] = { NULL };
            kernel_execve("/usr/bin/hello", argvec, envvec);
            return;
        }
        int hello_test(kshell_t *kshell, int argc, char **argv)
        {
            KASSERT(kshell != NULL);
            //dbg(DBG_PRINT, "(GRADING1D 2): vfs_test() is invoked, argc = %d, argv = 0x%08x\n",
                    //argc, (unsigned int)argv);
            int rv;
            /*
             * Shouldn't call a test function directly.
             * It's best to invoke it in a separate kernel process.  
             */
            
            proc_thread_t pt;
            pt.p = proc_create("hello");
            pt.t = kthread_create(pt.p, (void *)test1, argc, argv);
            sched_make_runnable(pt.t);
            do_waitpid(pt.p->p_pid,0,&rv);
            return 0;
        }
        void test2(int arg1, void *arg2)
        {
            char *const argvec[] = {"/usr/bin/args","ab", "cde", "fghi", "j", NULL};
            do_open("/dev/tty0", O_RDONLY);
            do_open("/dev/tty0", O_WRONLY);
            char *const envvec[] = { NULL };
            kernel_execve("/usr/bin/args", argvec, envvec);
            return;
        }
        int args_test(kshell_t *kshell, int argc, char **argv)
        {
            KASSERT(kshell != NULL);
            //dbg(DBG_PRINT, "(GRADING1D 2): vfs_test() is invoked, argc = %d, argv = 0x%08x\n",
                    //argc, (unsigned int)argv);
            int rv;
            /*
             * Shouldn't call a test function directly.
             * It's best to invoke it in a separate kernel process.  
             */
            
            proc_thread_t pt;
            pt.p = proc_create("uname");
            pt.t = kthread_create(pt.p, (void *)test2, argc, argv);
            sched_make_runnable(pt.t);
            do_waitpid(pt.p->p_pid,0,&rv);
            return 0;
        }
        void test3(int arg1, void *arg2)
        {
            char *const argvec[] = {"/bin/uname","-a", NULL};
            do_open("/dev/tty0", O_RDONLY);
            do_open("/dev/tty0", O_WRONLY);
            char *const envvec[] = { NULL };
            kernel_execve("/bin/uname", argvec, envvec);
            return;
        }
        int uname_test(kshell_t *kshell, int argc, char **argv)
        {
            KASSERT(kshell != NULL);
            //dbg(DBG_PRINT, "(GRADING1D 2): vfs_test() is invoked, argc = %d, argv = 0x%08x\n",
                    //argc, (unsigned int)argv);
            int rv;
            /*
             * Shouldn't call a test function directly.
             * It's best to invoke it in a separate kernel process.  
             */
            
            proc_thread_t pt;
            pt.p = proc_create("uname");
            pt.t = kthread_create(pt.p, (void *)test3, argc, argv);
            sched_make_runnable(pt.t);
            do_waitpid(pt.p->p_pid,0,&rv);
            return 0;
        }
        void test4(int arg1, void *arg2)
        {
            char *const argvec[] = {"/bin/stat","/README", NULL};
            do_open("/dev/tty0", O_RDONLY);
            do_open("/dev/tty0", O_WRONLY);
            char *const envvec[] = { NULL };
            kernel_execve("/bin/stat", argvec, envvec);
            return;
        }
        int readme_test(kshell_t *kshell, int argc, char **argv)
        {
            KASSERT(kshell != NULL);
            //dbg(DBG_PRINT, "(GRADING1D 2): vfs_test() is invoked, argc = %d, argv = 0x%08x\n",
                    //argc, (unsigned int)argv);
            int rv;
            /*
             * Shouldn't call a test function directly.
             * It's best to invoke it in a separate kernel process.  
             */
            
            proc_thread_t pt;
            pt.p = proc_create("readme");
            pt.t = kthread_create(pt.p, (void *)test4, argc, argv);
            sched_make_runnable(pt.t);
            do_waitpid(pt.p->p_pid,0,&rv);
            return 0;
        }
        void test5(int arg1, void *arg2)
        {
            char *const argvec[] = {"/bin/stat","/usr", NULL};
            do_open("/dev/tty0", O_RDONLY);
            do_open("/dev/tty0", O_WRONLY);
            char *const envvec[] = { NULL };
            kernel_execve("/bin/stat", argvec, envvec);
            return;
        }
        int user_test(kshell_t *kshell, int argc, char **argv)
        {
            KASSERT(kshell != NULL);
            //dbg(DBG_PRINT, "(GRADING1D 2): vfs_test() is invoked, argc = %d, argv = 0x%08x\n",
                    //argc, (unsigned int)argv);
            int rv;
            /*
             * Shouldn't call a test function directly.
             * It's best to invoke it in a separate kernel process.  
             */
            
            proc_thread_t pt;
            pt.p = proc_create("user");
            pt.t = kthread_create(pt.p, (void *)test5, argc, argv);
            sched_make_runnable(pt.t);
            do_waitpid(pt.p->p_pid,0,&rv);
            return 0;
        }
        void test6(int arg1, void *arg2)
        {
            char *const argvec[] = {"/bin/ls","/usr/bin", NULL};
            //do_open("/dev/tty0", O_RDONLY);
            //do_open("/dev/tty0", O_WRONLY);
            char *const envvec[] = { NULL };
            kernel_execve("/bin/ls", argvec, envvec);
            return;
        }
        int ls_test(kshell_t *kshell, int argc, char **argv)
        {
            KASSERT(kshell != NULL);
            //dbg(DBG_PRINT, "(GRADING1D 2): vfs_test() is invoked, argc = %d, argv = 0x%08x\n",
                    //argc, (unsigned int)argv);
            int rv;
            /*
             * Shouldn't call a test function directly.
             * It's best to invoke it in a separate kernel process.  
             */
            
            proc_thread_t pt;
            pt.p = proc_create("ls");
            pt.t = kthread_create(pt.p, (void *)test6, argc, argv);
            sched_make_runnable(pt.t);
            do_waitpid(pt.p->p_pid,0,&rv);
            return 0;
        }
        void test7(int arg1, void *arg2)
        {
            char *const argvec[] = {"/usr/bin/vfstest", NULL};
            //do_open("/dev/tty0", O_RDONLY);
            //do_open("/dev/tty0", O_WRONLY);
            char *const envvec[] = { NULL };
            kernel_execve("/usr/bin/vfstest", argvec, envvec);
            return;
        }
        int vfs(kshell_t *kshell, int argc, char **argv)
        {
            KASSERT(kshell != NULL);
            //dbg(DBG_PRINT, "(GRADING1D 2): vfs_test() is invoked, argc = %d, argv = 0x%08x\n",
                    //argc, (unsigned int)argv);
            int rv;
            /*
             * Shouldn't call a test function directly.
             * It's best to invoke it in a separate kernel process.  
             */
            
            proc_thread_t pt;
            pt.p = proc_create("vfs");
            pt.t = kthread_create(pt.p, (void *)test7, argc, argv);
            sched_make_runnable(pt.t);
            do_waitpid(pt.p->p_pid,0,&rv);
            return 0;
        }
        void test8(int arg1, void *arg2)
        {
            char *const argvec[] = {"/usr/bin/memtest", NULL};
            //do_open("/dev/tty0", O_RDONLY);
            //do_open("/dev/tty0", O_WRONLY);
            char *const envvec[] = { NULL };
            kernel_execve("/usr/bin/memtest", argvec, envvec);
            return;
        }
        int mem_test(kshell_t *kshell, int argc, char **argv)
        {
            KASSERT(kshell != NULL);
            //dbg(DBG_PRINT, "(GRADING1D 2): vfs_test() is invoked, argc = %d, argv = 0x%08x\n",
                    //argc, (unsigned int)argv);
            int rv;
            /*
             * Shouldn't call a test function directly.
             * It's best to invoke it in a separate kernel process.  
             */
            
            proc_thread_t pt;
            pt.p = proc_create("memtest");
            pt.t = kthread_create(pt.p, (void *)test8, argc, argv);
            sched_make_runnable(pt.t);
            do_waitpid(pt.p->p_pid,0,&rv);
            return 0;
        }
        void test9(int arg1, void *arg2)
        {
            char *const argvec[] = {"/usr/bin/eatmem", NULL};
            //do_open("/dev/tty0", O_RDONLY);
            //do_open("/dev/tty0", O_WRONLY);
            char *const envvec[] = { NULL };
            kernel_execve("/usr/bin/eatmem", argvec, envvec);
            return;
        }
        int eat_test(kshell_t *kshell, int argc, char **argv)
        {
            KASSERT(kshell != NULL);
            //dbg(DBG_PRINT, "(GRADING1D 2): vfs_test() is invoked, argc = %d, argv = 0x%08x\n",
                    //argc, (unsigned int)argv);
            int rv;
            /*
             * Shouldn't call a test function directly.
             * It's best to invoke it in a separate kernel process.  
             */
            
            proc_thread_t pt;
            pt.p = proc_create("eattest");
            pt.t = kthread_create(pt.p, (void *)test9, argc, argv);
            sched_make_runnable(pt.t);
            do_waitpid(pt.p->p_pid,0,&rv);
            return 0;
        }
        void test10(int arg1, void *arg2)
        {
            char *const argvec[] = {"/usr/bin/forkbomb", NULL};
            //do_open("/dev/tty0", O_RDONLY);
            //do_open("/dev/tty0", O_WRONLY);
            char *const envvec[] = { NULL };
            kernel_execve("/usr/bin/forkbomb", argvec, envvec);
            return;
        }
        int fork_test(kshell_t *kshell, int argc, char **argv)
        {
            KASSERT(kshell != NULL);
            //dbg(DBG_PRINT, "(GRADING1D 2): vfs_test() is invoked, argc = %d, argv = 0x%08x\n",
                    //argc, (unsigned int)argv);
            int rv;
            /*
             * Shouldn't call a test function directly.
             * It's best to invoke it in a separate kernel process.  
             */
            
            proc_thread_t pt;
            pt.p = proc_create("forkbomb");
            pt.t = kthread_create(pt.p, (void *)test10, argc, argv);
            sched_make_runnable(pt.t);
            do_waitpid(pt.p->p_pid,0,&rv);
            return 0;
        }
        void test11(int arg1, void *arg2)
        {
            char *const argvec[] = {"/usr/bin/stress", NULL};
            //do_open("/dev/tty0", O_RDONLY);
            //do_open("/dev/tty0", O_WRONLY);
            char *const envvec[] = { NULL };
            kernel_execve("/usr/bin/stress", argvec, envvec);
            return;
        }
        int stress_test(kshell_t *kshell, int argc, char **argv)
        {
            KASSERT(kshell != NULL);
            //dbg(DBG_PRINT, "(GRADING1D 2): vfs_test() is invoked, argc = %d, argv = 0x%08x\n",
                    //argc, (unsigned int)argv);
            int rv;
            /*
             * Shouldn't call a test function directly.
             * It's best to invoke it in a separate kernel process.  
             */
            
            proc_thread_t pt;
            pt.p = proc_create("stress");
            pt.t = kthread_create(pt.p, (void *)test11, argc, argv);
            sched_make_runnable(pt.t);
            do_waitpid(pt.p->p_pid,0,&rv);
            return 0;
        }
        void test12(int arg1, void *arg2)
        {
            char *const argvec[] = { NULL};
            //do_open("/dev/tty0", O_RDONLY);
            //do_open("/dev/tty0", O_WRONLY);
            char *const envvec[] = { NULL };
            kernel_execve("/usr/bin/fork-and-wait", argvec, envvec);
            return;
        }
        int fork_and_wait_test(kshell_t *kshell, int argc, char **argv)
        {
            KASSERT(kshell != NULL);
            //dbg(DBG_PRINT, "(GRADING1D 2): vfs_test() is invoked, argc = %d, argv = 0x%08x\n",
                    //argc, (unsigned int)argv);
            int rv;
            /*
             * Shouldn't call a test function directly.
             * It's best to invoke it in a separate kernel process.  
             */
            
            proc_thread_t pt;
            pt.p = proc_create("fork-and-wait");
            pt.t = kthread_create(pt.p, (void *)test12, argc, argv);
            sched_make_runnable(pt.t);
            do_waitpid(pt.p->p_pid,0,&rv);
            return 0;
        }
        void test13(int arg1, void *arg2)
        {
            char *const argvec[] = { NULL};
            //do_open("/dev/tty0", O_RDONLY);
            //do_open("/dev/tty0", O_WRONLY);
            char *const envvec[] = { NULL };
            kernel_execve("/sbin/init", argvec, envvec);
            return;
        }
        int user_space_test(kshell_t *kshell, int argc, char **argv)
        {
            KASSERT(kshell != NULL);
            //dbg(DBG_PRINT, "(GRADING1D 2): vfs_test() is invoked, argc = %d, argv = 0x%08x\n",
                    //argc, (unsigned int)argv);
            int rv;
            /*
             * Shouldn't call a test function directly.
             * It's best to invoke it in a separate kernel process.  
             */
            
            proc_thread_t pt;
            pt.p = proc_create("user_space");
            pt.t = kthread_create(pt.p, (void *)test13, argc, argv);
            sched_make_runnable(pt.t);
            do_waitpid(pt.p->p_pid,0,&rv);
            return 0;
        }



#endif /* __DRIVERS__ */
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
static void *
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
        //NOT_YET_IMPLEMENTED("VFS: idleproc_run");
        curproc->p_cwd = vfs_root_vn;
        vref(vfs_root_vn);

        initthr->kt_proc->p_cwd = vfs_root_vn;
        //vref(vfs_root_vn);

        do_mkdir("/dev");

        /* Here you need to make the null, zero, and tty devices using mknod */
        /* You can't do this until you have VFS, check the include/drivers/dev.h
         * file for macros with the device ID's you will need to pass to mknod */
        //NOT_YET_IMPLEMENTED("VFS: idleproc_run");

        do_mknod("/dev/null",S_IFCHR,MEM_NULL_DEVID);
        do_mknod("/dev/zero",S_IFCHR,MEM_ZERO_DEVID);
        do_mknod("/dev/tty0",S_IFCHR,MKDEVID(2,0));
        //do_mknod("/dev/tty1",S_IFCHR,MKDEVID(2,1));
#endif

        /* Finally, enable interrupts (we want to make sure interrupts
         * are enabled AFTER all drivers are initialized) */
        intr_enable();

        /* Run initproc */
        sched_make_runnable(initthr);
        /* Now wait for it */
        child = do_waitpid(-1, 0, &status);
        KASSERT(PID_INIT == child);

#ifdef __MTP__
        kthread_reapd_shutdown();
#endif


#ifdef __SHADOWD__
        /* wait for shadowd to shutdown */
        shadowd_shutdown();
#endif

#ifdef __VFS__
        /* Shutdown the vfs: */
        dbg_print("weenix: vfs shutdown...\n");
        vput(curproc->p_cwd);
        if (vfs_shutdown())
                panic("vfs shutdown FAILED!!\n");

#endif

        /* Shutdown the pframe system */
#ifdef __S5FS__
        pframe_shutdown();
#endif

        dbg_print("\nweenix: halted cleanly!\n");
        GDB_CALL_HOOK(shutdown);
        hard_shutdown();
        return NULL;
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
static kthread_t *
initproc_create(void)
{
        /*NOT_YET_IMPLEMENTED("PROCS: initproc_create");*/
        proc_thread_t pt;

        pt.p = proc_create("init");
        pt.t = kthread_create(pt.p, initproc_run, 0, NULL);
        KASSERT(NULL != pt.p);
        KASSERT(PID_INIT == pt.p->p_pid);
        KASSERT(NULL != pt.t);
        dbg(DBG_PRINT, "(GRADING1A 1.b)\n");
        return pt.t;
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
static void *
initproc_run(int arg1, void *arg2)
{
        /*NOT_YET_IMPLEMENTED("PROCS: initproc_run");*/
        //kthread_create(curproc, vfstest_main, 1, NULL);
        //char *const argvec[] = {"/usr/bin/vfstest", NULL };
        //char *const argvec[] = { NULL };
        //char *const argvec[] = {"/usr/bin/args","ab", "cde", "fghi", "j", NULL};
        //do_open("/dev/tty0", O_RDONLY);
        //do_open("/dev/tty0", O_WRONLY);
        //char *const argvec[] = {"/bin/uname","-a", NULL };
        //char *const argvec[] = {"/bin/stat","/README", NULL };
        //char *const argvec[] = {"/bin/stat","/usr", NULL };
        //char *const argvec[] = {"/bin/ls","/usr/bin", NULL };
        //char *const envvec[] = { NULL };
        //kernel_execve("/usr/bin/hello", argvec, envvec);
        //kernel_execve("/usr/bin/args", argvec, envvec);
        //kernel_execve("/bin/uname", argvec, envvec);
        //kernel_execve("/bin/stat", argvec, envvec);
        //kernel_execve("/bin/stat", argvec, envvec);
        //kernel_execve("/bin/ls", argvec, envvec);
        //kernel_execve("/usr/bin/fork-and-wait", argvec, envvec);
        //kernel_execve("/sbin/init", argvec, envvec);
        //kernel_execve("/usr/bin/vfstest", argvec, envvec);
        #ifdef __DRIVERS__

        kshell_add_command("sunghan", my_sunghan_test, "Run sunghan_test().");
        kshell_add_command("deadlock", my_sunghan_deadlock_test, "Run sunghan_deadlock_test().");
        kshell_add_command("faber", my_faber_thread_test, "Run faber_thread_test().");
        kshell_add_command("vfstest", my_vfs_test, "Run vfstest_main().");
        kshell_add_command("thrtest", faber_fs_thread_test, "Run faber_fs_thread_test().");
        kshell_add_command("dirtest", faber_directory_test, "Run faber_directory_test().");
        kshell_add_command("hello", hello_test, "Run hello_test().");
        kshell_add_command("args", args_test, "Run args_test().");
        kshell_add_command("uname", uname_test, "Run uname_test().");
        kshell_add_command("readme", readme_test, "Run readme_test().");
        kshell_add_command("user", user_test, "Run user_test().");
        kshell_add_command("ls", ls_test, "Run ls_test().");
        kshell_add_command("fork-and-wait", fork_and_wait_test, "Run fork_and_wait_test().");
        kshell_add_command("vmvfs", vfs, "Run vfs().");
        kshell_add_command("memtest", mem_test, "Run mem_test().");
        kshell_add_command("eattest", eat_test, "Run eat_test().");
        kshell_add_command("forkbomb", fork_test, "Run fork_test().");
        kshell_add_command("stress", stress_test, "Run stress_test().");
        kshell_add_command("sbin", user_space_test, "Run user_space_test().");


        kshell_t *kshell = kshell_create(0);
        if (NULL == kshell) panic("init: Couldn't create kernel shell\n");
        dbg(DBG_PRINT, "(GRADING1B)\n");
        while (kshell_execute_next(kshell));
        kshell_destroy(kshell);
        #endif /* __DRIVERS__ */
        return NULL;
}
