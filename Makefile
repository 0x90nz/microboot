CFLAGS=-m32 -nostdlib -ffreestanding -fno-pie
CC=gcc

image: stage2 loader
	dd if=/dev/zero of=build/load.img bs=512 count=2880
	dd if=build/load.bin of=build/load.img conv=notrunc
	dd if=build/stage2.bin of=build/load.img bs=1 seek=512 conv=notrunc

.suffixes: .o .c

.PHONY: loader
loader:
	nasm -f bin -o build/load.bin loader/load.S

stage2: kern/kernel.o kern/vga.o kern/keyboard.o
	$(CC) $(CFLAGS) -c loader/stage2.S -o build/stage2.o
	$(CC) $(CFLAGS) build/stage2.o $(addprefix build/, $(notdir $^)) -T link.ld -o build/stage2.bin

.c.o:
	$(CC) $(CFLAGS) -c $< -o build/$(notdir $@)

run: image
	qemu-system-i386 -drive format=raw,file=build/load.img,index=0,if=floppy -serial mon:stdio

clean:
	rm -r build
