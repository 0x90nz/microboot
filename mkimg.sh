#!/bin/sh
IMAGE="$1"

if $(mkdir mnt) ; then
    dd if=/dev/zero of="$IMAGE" bs=1M count=32
    echo "o
    n
    p
    1
    2048

    a
    w" | fdisk "$IMAGE" >/dev/null

    LOOP=$(sudo losetup -fP --show "$IMAGE")
    sudo mkfs.ext2 "${LOOP}p1"
    sudo mount "${LOOP}p1" mnt
    sudo cp -r rootfs/* mnt
    sudo umount mnt
    sudo losetup -d "$LOOP"
    rmdir mnt
else
    echo "Cannot finish, mnt already present. Please clean up"
    exit -1
fi