<h1>A simple operating system kernel - SOS</h1>
SOS - hobby kernel for x86-64 compatible systems. </br>
Written using C and Assembly. GCC is used as C compiler, NASM is used as assembler. </br>
Kernel uses Multiboot 2 specification and GRUB 2 as bootloader. </br>

<h3>This kernel for now works only in CSM(compatibility support mode) since it uses deprecated VGA 80x25 text mode (because its easy to get up and running). Main branch has been tested on real hardware.</h3>

<h2>A bit of implementation details:</h2>
1. Kernel is remapped into higher half of virtual address space, and stays there all of the time, so user space programs can start from beginning of virtual address space and do syscalls.</br>
2. All available ram is identity mapped into kernel space (just like Linux does).</br>
3. Kernel heap is based on Doug Lea's malloc for glibc, though is heavily simplified.</br>
4. Since in x64 segmentation is deprecated, but identity segments are still required, GDT is set up to bare minimum.</br>

<h2>Current memory map:</h2>
<pre>
//=======================================================================================
//    Start addr    | Offset   |     End addr     | Description
//=======================================================================================
//                  |          |                  |
// 0000000000000000 |  0       | 00007fffffffffff | user-space virtual memory
//__________________|__________|__________________|______________________________________
//                  |          |                  |
// 0000800000000000 | +128  TB | ffff7fffffffffff | non-canonical virtual memory
//__________________|__________|__________________|______________________________________
//                                                |
//                                                | Kernel-space virtual memory
//________________________________________________|______________________________________
//                  |          |                  |
// ffff800000000000 | -128  TB | ffff87ffffffffff | Kernel code
// ffff888000000000 | -119.5TB | ffffc87fffffffff | Direct mapping of all physical memory
// ffffc88000000000 | -55.5 TB |                  | Kernel heap
//__________________|__________|__________________|______________________________________
</pre>

<h2>Feature set</h2>

What is implemented:
- identity segmentation
- basic irq handling, PIC set up
- multiboot info provided by bootloader parsing
- timer set up
- physical memory management
- virtual memory management
- basic atomic operations, spin lock support
- kernel heap (kmalloc, kmalloc_aligned, krealloc, kfree)
- vga text console

TBD:
- preemptive multhreading, ring 0 <- right now working on this part
- group threads to form processes, ring 0
- processes, threads, ring 3
- more synchronization primitives apart from basic spinlocks - mutex, semaphores, conditional variables
- syscall interface and basic unix syscalls implementation
- modern graphics interface support to get rid of CSM
- interface for drivers
- simple hdd driver, file system
- SMP(simultaneous multi-processing) support
- compile portable std lib for C for kernel

<h2>Building and running</h2>

To build and run in QEMU:
1. Clone git repository. </br>
2. Change `CROSS_COMPILE` prefix in Makefile to your toolchain prefix. </br>
3. Run `make all` from root repository directory. </br>
4. `build_output` folder now contains `sos.iso` file. </br>
5. Run `make run` to run on QEMU emulator.

To make bootable flash drive, after step 4:
1. Run `lsblk` and get name of your flashdrive (for example `sdb`) </br>
2. Unmount all partitions of selected flashdrive with `sudo umount`(for example `sudo umount /dev/sdb1 /dev/sdb2 /dev/sdb3 /dev/sdb4`).
3. Copy iso image of kernel with `dd` (for example `sudo dd if=build_output/sos.iso of=/dev/sdb && sync`)
