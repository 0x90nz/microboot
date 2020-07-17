CFLAGS=-m32 -march=i386 -nostdlib -ffreestanding -fno-pie -mgeneral-regs-only -mno-red-zone -msoft-float
CC=gcc

KSRC=$(shell find kern -type f -name "*.c")
KOBJS=$(KSRC:.c=.o)

image: user stage2 loader
	dd if=/dev/zero of=build/load.img bs=512 count=2880
	dd if=build/load.bin of=build/load.img conv=notrunc
	dd if=build/stage2.bin of=build/load.img bs=1 seek=512 conv=notrunc

.suffixes: .o .c

.PHONY: loader
loader:
	nasm -f bin -o build/load.bin loader/load.S

.PHONY: user
user:
	$(CC) $(CFLAGS) -c user/main.c -o build/main.o -Ikern

thing:
	echo $(KOBJS)

stage2: $(KOBJS)
	$(CC) $(CFLAGS) -c loader/stage2.S -o build/stage2.o
	$(CC) $(CFLAGS) -c kern/interrupts_stubs.S -o build/interrupts_stubs.o
	$(CC) $(CFLAGS) build/stage2.o build/interrupts_stubs.o build/main.o $(addprefix build/, $(notdir $^)) -T link.ld -o build/stage2.bin

.c.o:
	$(CC) $(CFLAGS) -c $< -o build/$(notdir $@)

run: image
	qemu-system-i386 \
		-drive format=raw,file=build/load.img,index=0,if=floppy \
		-serial mon:stdio \
		-netdev hubport,hubid=1,id=n1,id=eth -device ne2k_pci,netdev=n1,mac=de:ad:be:ef:c0:fe \
		-object filter-dump,id=id,netdev=n1,file=out.pcap

clean:
	rm -r build
