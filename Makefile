image: loader stage2
	dd if=/dev/zero of=load.img bs=512 count=2880
	dd if=load.bin of=load.img conv=notrunc
	dd if=stage2.bin of=load.img bs=1 seek=512 conv=notrunc


loader:
	nasm -f bin -o load.bin load.S

stage2:
	gcc -m16 -nostdinc -nostdlib stage2.S -T link.ld -o stage2.bin

run: image
	qemu-system-i386 -drive format=raw,file=load.img,index=0,if=floppy -serial mon:stdio

clean:
	rm load.img load.bin
