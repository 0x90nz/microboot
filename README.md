# μboot
A small (and very limited) bootloader

## What is it?

μboot is a small environment that allows a C program to run on "bare metal". It
does not require the typical `i686-elf-gcc` (or similar) cross compiler, and is
designed to work with a normal installation of `gcc` on an x86_64 computer. This
solution is somewhat unstable however, and is only used because it greatly
reduces the complexity to get started.

## How to use

To use μboot, you'll at the very minimum need some linux environment, which has
the tools `qemu`, `gcc`, `nasm`, and `dd` available. Once all the tools are
installed, you can run `make` to build or `make run` to both build and launch
qemu.

By default a 1.44MiB floppy disk image is produced, although in theory because
BIOS interrupts are used, it μboot should run just as well from a hard disk or
any other bootable media.

## Architecture

There are 3 main components:

 1. First stage loader (boot sector)
 2. Second stage loader
 3. User program

### First Stage Loader

The first stage loader is implemented in `nasm` as it better supports the 16bit
code that is require when the machine is initialised. It is stored within the
boot sector of a drive (either a floppy or hard disk image). The first stage
loader uses BIOS interrupts to load the second stage loader from the disk that
has been booted.

### Second Stage Loader

The second stage takes care of the platform initialisation. This includes
enabling the A20 gate, setting up a very simple GDT, and switching to protected
mode. The second stage is written in gnu `as` assembly (AT&T syntax).

### User program

This is the program which is given control once the second stage has set
everything up. The entry point is `kernel_main`. This sets up the devices and
initialises the IDT so that interrupts can be used. Once setup is done, it jumps
to `main`, which should be defined in `user/main.c`. This is the actual user
program.

## Environment

At the moment a _very_ limited subset of functions inspired by (but not
necessarily compatible with) the POSIX standard library are provided in
`stdlib.h`. In addition the devices and kernel functions are accessible by their
respective headers.