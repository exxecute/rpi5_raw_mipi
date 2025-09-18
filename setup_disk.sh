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
sudo mount /dev/sdc1 mnt/boot
sudo mount /dev/sdc2 mnt/root

# Install the kernel modules onto the boot media
sudo env PATH=$PATH make -j12 ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- INSTALL_MOD_PATH=mnt/root modules_install

# Backup image of the current kernel, install the fresh kernel image, overlays, README
sudo cp mnt/boot/$KERNEL.img mnt/boot/$KERNEL-backup.img
sudo cp arch/arm64/boot/Image mnt/boot/$KERNEL.img
sudo cp arch/arm64/boot/dts/broadcom/*.dtb mnt/boot/
sudo cp arch/arm64/boot/dts/overlays/*.dtb* mnt/boot/overlays/
sudo cp arch/arm64/boot/dts/overlays/README mnt/boot/overlays/

# Umount sd_card
sudo umount mnt/boot
sudo umount mnt/root
