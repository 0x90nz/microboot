CFLAGS=-nostdlib
CC=gcc

image: stage2 loader
	dd if=/dev/zero of=load.img bs=512 count=2880
	dd if=load.bin of=load.img conv=notrunc
	dd if=stage2.bin of=load.img bs=1 seek=512 conv=notrunc

.PHONY: loader
loader:
	nasm -f bin -o load.bin loader/load.S

stage2:
	$(CC) $(CFLAGS) -c loader/stage2.S -o stage2.o
	$(CC) $(CFLAGS) -c kernel.c -o kernel.o
	$(CC) $(CFLAGS) stage2.o kernel.o -T link.ld -o stage2.bin

run: image
	qemu-system-i386 -drive format=raw,file=load.img,index=0,if=floppy -serial mon:stdio

clean:
	rm load.img load.bin *.o
