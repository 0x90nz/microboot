# μboot
A small bootloader and operating environment

![μboot on boot](.images/microboot_1.png)

## What is it?

μboot is a small environment that allows a C program to run on "bare metal". It
does not require the typical `i686-elf-gcc` (or similar) cross compiler, and is
designed to work with a normal installation of `gcc` on an x86_64 computer. This
solution is somewhat unstable however, and is only used because it greatly
reduces the complexity to get started.

## How to use

To use μboot, you'll at the very minimum need some Linux (or possibly Unix if
you can find the tools) environment, which has the tools `qemu`, `gcc`, `nasm`,
`mtools` and `dd` available (some of these are very likely installed on some
systems already). Note that on some distributions you might have to install a
package like `gcc-multilib` to get support for compiling in 32-bit mode with
x86_64 `gcc`.Once all the tools are installed, you can run `make` to build or
`make run` to both build and launch qemu.

By default a hard disk image is created, 32MiB large (see `mkimg.sh` for more
details). The bootloader's core image is stored within the space between the MBR
and the first partition.

To debug, just type `make debug` which will drop you to a `gdb` prompt (using
QEMU's gdb stub), this of course needs `gdb` installed. Once here you can do
most of the things you'd normally be able to do with a C program, like set
breakpoints (e.g. `b main`) etc.

## Architecture

There are 4 main components:

 1. First stage loader (boot sector)
 2. Second stage loader
 3. Main program
 4. User programs

The second stage loader and main program are linked together into one binary
file at compile time. The main program currently implements a minimal command
line.

The user programs are position independent ELF files which are compiled
and may be loaded from disk at runtime. To use symbols from the kernel, or export
their own symbols for use by the kernel the `export.h` header must be used. See
this file for more detail on how symbols are handled.

### First Stage Loader

The first stage loader is implemented in `nasm` as it better supports the 16bit
code that is required when the machine is initialised. It is stored within the
boot sector of a drive (either a floppy or hard disk image). The first stage
loader uses BIOS interrupts to load the second stage loader from the disk that
has been booted.

### Second Stage Loader

The second stage takes care of the platform initialisation. This includes
enabling the A20 gate, setting up a very simple GDT, and switching to protected
mode. The second stage is written in gnu `as` assembly (AT&T syntax).

### Main program

Once all the devices are setup, execution will be transferred to the `main`
function. This is defined in `kern/main.c`. From here user programs can be
loaded and executed through the `exec` command.

### User programs

User programs are loaded and executed at runtime. They may either be a module,
or an executable. Both types follow the same general format.

Any kernel symbols to use must be declared and initialised. This is done with
the `USE` macro for declaration and the `INIT` macro for initialisation. In this
way the kernel does not need to do any linking before loading, this is taken
care of by the user program. This does add extra complication to the user
program, but it simplifies the kernel design.

## Environment

At the moment a _very_ limited subset of functions inspired by (but not
necessarily compatible with) the POSIX standard library are provided in
`stdlib.h`. In addition the devices and kernel functions are accessible by their
respective headers.

## Philosophy

Once control is handed over to the user program, it can do whatever it wants
(including crashing the system, intentionally or unintentionally). It is not a
'user' program in the traditional sense of an operating system, where it would
run at a lower privilege level and perhaps be subject to preemption. It is
simply a user defined program that can be run.

A core set of functions are provided in the standard library, and in the rest of
the kernel. These can be exported to be used in the user program. The functions
provided allow for convenient use of peripherals, as well as being able to
complete common tasks but they're in no way mandatory. It would be entirely
possible to rewrite all of these within the user program and only use those
local copies.

## Documentation

I'm using this project as an opportunity to learn about
[Doxygen](https://www.doxygen.nl). Documentation is being written alongside new
code, and old code is being documented as and where I find time. Documentation
can be generated by running `doxygen Doxyfile` in the root directory, and output
can be viewed in the `doc` folder after this. At the moment the front page is
empty, so the most useful place to go is `files.html` which provides a starting
point to explore the documentation.

## Acknowledgements

This project uses mpaland's printf library, which is under the MIT license. To
learn more about this project go to the repository page
[here](https://github.com/mpaland/printf).
