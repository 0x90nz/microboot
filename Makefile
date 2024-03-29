
GIT_REV=$(shell git describe --match="" --always --dirty)
GIT_BRANCH=$(shell git symbolic-ref --short HEAD)

# we disable the frame-address warning as we only use it for backtrace when debugging
# so the possiblility of unstable behaviour doesn't concern us, as the system has already
# crashed!
CFLAGS=-m32 -march=i386 -nostdlib -nostdinc -ffreestanding -fno-pie -fno-stack-protector \
 -mgeneral-regs-only -mno-red-zone -msoft-float -Wall -Wno-frame-address \
 -fno-asynchronous-unwind-tables -Ilib \
 -DVER_GIT_REV="\"$(GIT_REV)\"" -DVER_GIT_BRANCH="\"$(GIT_BRANCH)\""
CC=gcc
QEMU="qemu-system-x86_64"
QEMU_32="qemu-system-i386"

KSRC=$(shell find kern -type f -name "*.c")
BUILD=build
KOBJS=$(addprefix $(BUILD)/, $(KSRC:%.c=%.o))

.PHONY: .FORCE
.FORCE:


# wrapper so that image gets called by default
.PHONY: all
all: image

# XXX: not great - force main to be rebuilt so version string gets updated
#      this should eventually be changed to use function calls to get version
#      so that just something like version.c can be forced to be rebuilt #38
build/kern/main.o: .FORCE

.PHONY: build_dir
build_dir:
	mkdir -p $(BUILD)

.PHONY: rootfs_dir
rootfs_dir:
	mkdir -p rootfs/bin

.PHONY: loader
loader:
	nasm -f bin -o build/load.bin loader/load.S

.PHONY: user
user: rootfs_dir user/dino.elf

.PHONY: stage2
stage2: $(KOBJS)
	$(CC) $(CFLAGS) -c loader/stage2.S -o build/stage2.o
	$(CC) $(CFLAGS) -c kern/sys/bios.S -o build/bios.o
	$(CC) $(CFLAGS) -c loader/stage2_hl.c -o build/stage2_hl.o
	$(CC) $(CFLAGS) -c kern/sys/interrupts_stubs.S -o build/interrupts_stubs.o
	$(CC) $(CFLAGS) -lgcc build/stage2_hl.o build/interrupts_stubs.o build/bios.o \
		$^ -T link.ld -Wl,-Map=build/stage2.map -o build/stage2.bin

$(KOBJS): $(BUILD)/%.o: %.c
	mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@

%.elf: %.c
	$(CC) $(CFLAGS) -static -fPIC $< user/crt0.S -o rootfs/bin/$(notdir $@) -T user/process.ld -Ilib -Ikern

$(BUILD)/debugimg.elf: $(KOBJS)
	$(CC) $(CFLAGS) -lgcc $(BUILD)/stage2_hl.o $(BUILD)/interrupts_stubs.o $(BUILD)/bios.o \
		$^ -T link.ld -Wl,--oformat=elf32-i386 -o $(BUILD)/debugimage.elf

.PHONY: image
image: build_dir user stage2 loader
	./mkimg.sh build/microboot.img

.PHONY: debugimg
debugimage: build_dir stage2 loader $(BUILD)/debugimg.elf

.PHONY: run
run: CLFAGS += -O2
run: image
	$(QEMU) \
		-drive format=raw,file=build/microboot.img,index=0 \
		-serial mon:stdio \
		-netdev hubport,hubid=1,id=n1,id=eth -device ne2k_pci,netdev=n1,mac=de:ad:be:ef:c0:fe \
		-object filter-dump,id=id,netdev=n1,file=out.pcap

.PHONY: gdbdebug
gdbdebug: CFLAGS += -g
gdbdebug: image debugimage
	$(QEMU_32) \
		-drive format=raw,file=build/microboot.img,index=0 \
		-serial mon:stdio \
		-netdev hubport,hubid=1,id=n1,id=eth -device ne2k_pci,netdev=n1,mac=de:ad:be:ef:c0:fe \
		-object filter-dump,id=id,netdev=n1,file=out.pcap -s -S -d cpu_reset &

.PHONY: debug
debug: gdbdebug
	gdb -ex 'target remote localhost:1234' -ex 'symbol-file $(BUILD)/debugimage.elf'

.PHONY: clean
clean:
	rm -r build
