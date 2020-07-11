CFLAGS=-nostdlib
CC=gcc

image: stage2 loader
	dd if=/dev/zero of=build/load.img bs=512 count=2880
	dd if=build/load.bin of=build/load.img conv=notrunc
	dd if=build/stage2.bin of=build/load.img bs=1 seek=512 conv=notrunc

.PHONY: loader
loader:
	nasm -f bin -o build/load.bin loader/load.S

stage2:
	mkdir -p build
	$(CC) $(CFLAGS) -c loader/stage2.S -o build/stage2.o
	$(CC) $(CFLAGS) -c kernel.c -o build/kernel.o
	$(CC) $(CFLAGS) build/stage2.o build/kernel.o -T link.ld -o build/stage2.bin

run: image
	qemu-system-i386 -drive format=raw,file=build/load.img,index=0,if=floppy -serial mon:stdio

clean:
	rm -r build
