#!/bin/sh
IMAGE="$1"
FS_SIZE="30M"
BS_SIZE=$(stat -c "%s" build/load.bin)

if [ $BS_SIZE -gt 446 ] ; then
    tput setaf 3
    echo "WARNING: bootsector is more than 446 bytes ($BS_SIZE) may overflow onto partition table!"
    tput sgr0
fi

rm -f "$IMAGE"
dd if=/dev/zero of="$IMAGE" bs=1M count=32
echo "o
    n
    p
    1
    2048
    +$FS_SIZE
    a
    w" | fdisk "$IMAGE" >/dev/null

dd if="build/load.bin" of="$IMAGE" conv=notrunc
dd if="build/stage2.bin" of="$IMAGE" bs=1 seek=512 conv=notrunc

rm -f "build/rootfs.img"
mke2fs -b 1024 -L 'microboot' -d rootfs -t ext2 "build/rootfs.img" "$FS_SIZE"
dd if="build/rootfs.img" of="$IMAGE" bs=512 seek=2048 conv=notrunc
