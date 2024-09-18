# Weenix OS Project

This repository contains my implementation of several assignments for the Weenix operating system, a Unix-based educational OS developed at Brown University for learning about operating systems internals. These assignments involved implementing key parts of the kernel such as process and thread management, the virtual file system (VFS), and virtual memory (VM).

## Introduction to Weenix

Weenix is a small operating system kernel that supports basic functionality similar to Unix. It includes:

- Process and thread management
- A virtual file system (VFS) layer
- Device drivers
- Virtual memory (VM) management

The goal of Weenix is to help students understand how operating systems function at a low level by implementing various parts of the kernel. 

## Assignments Overview

This project covers three kernel assignments:

### 1. Kernel Assignment 1: Processes and Threads

In this assignment, I implemented basic process and thread management. This involved:

- Creating processes and threads
- Implementing the scheduler for switching between threads
- Synchronization primitives (mutexes)
- Handling thread/process exits, cleanup, and waitpid system calls

The functionality was tested by writing various test cases in `kmain.c` to ensure the proper behavior of thread/process lifecycle management and synchronization. These tests included `faber_thread_test()`, `sunghan_test()`, and `sunghan_deadlock_test()` to verify thread management and synchronization.

#### Steps to Compile and Run:
1. Set `DRIVERS=1` in `Config.mk`.
2. Build and run Weenix:
    ```bash
    make clean
    make
    ./weenix -n
    ```

3. Access `kshell` for testing and debugging. At the prompt, type `help` to see available commands.

### 2. Kernel Assignment 2: Virtual File System (VFS)

This assignment focused on implementing the Virtual File System (VFS) layer, which provides an abstraction to manage different file systems uniformly. The key components implemented include:

- Pathname to vnode conversion
- Opening and closing files
- Reference counting for vnodes and file descriptors
- Integration with `ramfs`, the provided in-memory file system

I added test cases in `kmain.c` and integrated commands like `vfstest_main()` in the shell for verification.

#### Additional Notes:
You can enable the VFS functionality by setting the following in `Config.mk`:
```bash
DRIVERS=1
VFS=1
```
Test Commands:
- `vfstest_main()` for testing VFS functionality
- `faber_fs_thread_test()` and `faber_directory_test()` for file system testing

### 3. Kernel Assignment 3: Virtual Memory (VM)

In the final assignment, I worked on the virtual memory subsystem to handle:

- Demand paging and page fault handling
- Memory mapping and page frame management
- Integration with the System V File System (S5FS)

The implementation of key memory management functions such as `pframe_get()`, `pframe_pin()`, and `pframe_unpin()` allowed for managing physical page frames. The memory management was tested using programs like `/usr/bin/hello` and custom commands in kshell.

#### Key Changes in Config.mk:
Set the following in `Config.mk` to enable the virtual memory system:
```bash
DRIVERS=1
VFS=1
S5FS=1
VM=1
```

#### Testing VM Functionality:
Run the user space shell `/sbin/init` using `kernel_execve()` to execute test programs in user space, such as `/usr/bin/hello`.

## Compilation and Running Weenix

To build and run the Weenix kernel:

1. Clone the Weenix source code into your local environment.
2. Set the necessary variables in `Config.mk` as described in each assignment section.
3. Run the following commands to build and run Weenix:
    ```bash
    make clean
    make
    ./weenix -n
    ```

Use `kshell` to run tests and validate functionality.

## Testing

For each assignment, I added commands to the `kshell` for testing:

- **Assignment 1**: Commands for thread and process tests, such as `faber_thread_test()`, `sunghan_test()`, and `sunghan_deadlock_test()`.
- **Assignment 2**: VFS-related tests using `vfstest_main()`, `faber_fs_thread_test()`, and `faber_directory_test()`.
- **Assignment 3**: Testing demand paging and VM functionality with programs like `/usr/bin/hello`.

These commands help ensure that the kernel behaves as expected across processes, file systems, and memory management.

## Conclusion

This project has been an invaluable learning experience in understanding how key components of an operating system—such as process management, file systems, and virtual memory—work. Weenix provides an excellent hands-on platform for exploring kernel-level programming and operating systems design.
