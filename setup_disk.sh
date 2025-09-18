#!/usr/bin/env bash

echo "Setting up image for RPI5 64-bit!"

KERNEL=kernel_2712

if [[ -v SD_CARD ]]; then
    echo '[OK]: SD_CARD sourced "$SD_CARD"'
else
    echo '[ERROR]: Source SD_CARD (export SD_CARD="/dev/sdxxx")'
fi

# create mnt dir
mkdir -p mnt
mkdir -p mnt/boot
mkdir -p mnt/root

# Mount sd_card
mount /dev/$SD_CARD1 mnt/boot
mount /dev/$SD_CARD2 mnt/root

# Install the kernel modules onto the boot media
env PATH=$PATH make -j12 ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- INSTALL_MOD_PATH=mnt/root modules_install

# Backup image of the current kernel, install the fresh kernel image, overlays, README
cp mnt/boot/$KERNEL.img mnt/boot/$KERNEL-backup.img
cp arch/arm64/boot/Image mnt/boot/$KERNEL.img
cp arch/arm64/boot/dts/broadcom/*.dtb mnt/boot/
cp arch/arm64/boot/dts/overlays/*.dtb* mnt/boot/overlays/
cp arch/arm64/boot/dts/overlays/README mnt/boot/overlays/

# Umount sd_card
umount mnt/boot
umount mnt/root
