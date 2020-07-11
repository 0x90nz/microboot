image: loader
	dd if=/dev/zero of=load.img bs=512 count=2880
	dd if=load.bin of=load.img conv=notrunc

loader:
	gcc -m16 -nostdinc -nostdlib load.S -T link.ld -o load.bin

run: image
	qemu-system-i386 -display none -fda load.img -serial mon:stdio

clean:
	rm load.img load.bin
