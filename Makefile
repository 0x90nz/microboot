CFLAGS=-m32 -march=i386 -nostdlib -nostdinc -ffreestanding -fno-pie \
 -mgeneral-regs-only -mno-red-zone -msoft-float -Wall -fno-asynchronous-unwind-tables -Ilib
CC=gcc

KSRC=$(shell find kern -type f -name "*.c")
KOBJS=$(KSRC:.c=.o)

image: build_dir user stage2 loader
	./mkimg.sh build/microboot.img
	dd if=build/load.bin of=build/microboot.img conv=notrunc
	dd if=build/stage2.bin of=build/microboot.img bs=1 seek=512 conv=notrunc

build_dir:
	mkdir -p build

.PHONY: loader
loader:
	nasm -f bin -o build/load.bin loader/load.S

.PHONY: user
user: user/info.elf user/dino.elf
	
stage2: $(KOBJS)
	$(CC) $(CFLAGS) -c loader/stage2.S -o build/stage2.o
	$(CC) $(CFLAGS) -c kern/sys/bios.S -o build/bios.o
	$(CC) $(CFLAGS) -c loader/stage2_hl.c -o build/stage2_hl.o
	$(CC) $(CFLAGS) -c kern/sys/interrupts_stubs.S -o build/interrupts_stubs.o
	$(CC) $(CFLAGS) -lgcc build/stage2_hl.o build/interrupts_stubs.o build/bios.o \
		$(addprefix build/, $(notdir $^)) -T link.ld -Wl,-Map=build/stage2.map -o build/stage2.bin

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o build/$(notdir $@)

%.elf: %.c
	$(CC) $(CFLAGS) -static -fPIC $< user/crt0.S -o rootfs/bin/$(notdir $@) -T user/process.ld -Ilib -Ikern

run: CLFAGS += -O3
run: image
	qemu-system-i386 \
		-drive format=raw,file=build/microboot.img,index=0 \
		-serial mon:stdio \
		-netdev hubport,hubid=1,id=n1,id=eth -device ne2k_pci,netdev=n1,mac=de:ad:be:ef:c0:fe \
		-object filter-dump,id=id,netdev=n1,file=out.pcap

debug: CFLAGS += -g
debug: image
	qemu-system-i386 \
		-drive format=raw,file=build/microboot.img,index=0 \
		-serial mon:stdio \
		-netdev hubport,hubid=1,id=n1,id=eth -device ne2k_pci,netdev=n1,mac=de:ad:be:ef:c0:fe \
		-object filter-dump,id=id,netdev=n1,file=out.pcap -s -S -d cpu_reset &
	konsole -e "gdb -ex 'target remote localhost:1234'"

clean:
	rm -r build
