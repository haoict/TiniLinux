#!/bin/bash
# Ref about commands: https://raspberrypi.stackexchange.com/questions/78466/how-to-make-an-image-file-from-scratch/78467#78467
# Check if user is root
if [[ $EUID -ne 0 ]]; then
    echo "This script must be run as root." 
    exit 1
fi

set -euo pipefail

# Load partition info variables
source board/${BOARD}/rootfs/root/partition-info.sh

OUT_IMG=output.${BOARD}/images/tinilinux-${BOARD}.img
rm -f ${OUT_IMG}

# mkflashableimg: Create an empty img file
echo "mkflashableimg: Create an empty img file"
truncate -s ${DISK_SIZE}M ${OUT_IMG}

# mkflashableimg: Make the disk MBR type (msdos)
echo "mkflashableimg: Make the disk MBR type (msdos)"
parted ${OUT_IMG} mktable msdos

if [[ "${BOARD}" == "rgb30"* ]]; then
    # mkflashableimg: Write the u-boot to the img (offset 64 sectors = 32KiB)
    echo "mkflashableimg: Write the u-boot to the img (offset 64 sectors = 32KiB)"
    dd if=output.${BOARD}/images/u-boot-rockchip.bin of=${OUT_IMG} bs=512 seek=64 conv=fsync,notrunc
elif [[ "${BOARD}" == "h700"* ]]; then
    # mkflashableimg: Write the u-boot to the img (offset 16 sectors = 8KiB)
    echo "mkflashableimg: Write the u-boot to the img (offset 16 sectors = 8KiB)"
    dd if=output.${BOARD}/images/u-boot-sunxi-with-spl.bin of=${OUT_IMG} bs=1K seek=8 conv=fsync,notrunc
else
    echo "u-boot writing not implemented for board ${BOARD}"
    exit 1
fi

# mkflashableimg: Making BOOT partitions
echo "mkflashableimg: Making BOOT partitions"
parted -s ${OUT_IMG} -a min unit s mkpart primary fat32 ${BOOT_PART_START} ${BOOT_PART_END}

# mkflashableimg: Making rootfs partitions
echo "mkflashableimg: Making rootfs partitions"
parted -s ${OUT_IMG} -a min unit s mkpart primary ext4 ${ROOTFS_PART_START} ${ROOTFS_PART_END}

# mkflashableimg: Set boot flag on the first partition
echo "mkflashableimg: Set boot flag on the first partition"
parted -s ${OUT_IMG} set 1 boot on
sync



# Unmount and Detach the img if previously mounted
echo "mkflashableimg: Unmount and Detach the img"
umount /mnt/BOOT || true
umount /mnt/rootfs || true
rm -rf /mnt/BOOT /mnt/rootfs
losetup --detach-all || true

# Format partitions and mount
echo "mkflashableimg: Format partitions and mount"
DEV_LOOP=$(losetup --show --find --partscan ${OUT_IMG})
mkfs.fat -F32 -n BOOT ${DEV_LOOP}p1
mkfs.ext4 -O ^orphan_file -L rootfs ${DEV_LOOP}p2

mkdir -p /mnt/BOOT && mount -t vfat ${DEV_LOOP}p1 /mnt/BOOT
mkdir -p /mnt/rootfs && mount -t ext4 ${DEV_LOOP}p2 /mnt/rootfs

# Copy kernel, initrd, dtb to /mnt/BOOT
echo "mkflashableimg: Copy kernel, initrd, dtb to /mnt/BOOT"
cp -r board/${BOARD}/BOOT/* /mnt/BOOT/
cp output.${BOARD}/images/Image /mnt/BOOT/
cp output.${BOARD}/images/initramfs /mnt/BOOT/
if [[ "${BOARD}" == "rgb30"* ]]; then
    cp -r output.${BOARD}/images/rockchip /mnt/BOOT/dtb
    cp output.${BOARD}/images/rk3566-dtbo/*.dtbo /mnt/BOOT/dtb/
elif [[ "${BOARD}" == "h700"* ]]; then
    cp -r output.${BOARD}/images/allwinner /mnt/BOOT/dtb
fi

# Extract buildroot (output.${BOARD}/images/rootfs.tar) to /mnt/rootfs
echo "mkflashableimg: Extract buildroot (output.${BOARD}/images/rootfs.tar) to /mnt/rootfs"
tar -xf output.${BOARD}/images/rootfs.tar -C /mnt/rootfs --no-same-owner
# Update build info: replace BUILD_ID=buildroot with BUILD_ID=yyyyMMdd-hhmmJST in /etc/os-release
sed -i "s/^BUILD_ID=buildroot/BUILD_ID=$(TZ='Asia/Tokyo' date +%Y%m%d-%H%M)JST/" /mnt/rootfs/etc/os-release
# Create roms.tar.xz to /root to be used in firstboot
mktempdir=$(mktemp -d)
cp -r board/common/ROMS/ ${mktempdir}/
if [ -d board/${BOARD}/ROMS/ ]; then cp -r board/${BOARD}/ROMS/ ${mktempdir}/; fi
tar -Jcf /mnt/rootfs/root/roms.tar.xz -C ${mktempdir}/ROMS/ .
rm -rf ${mktempdir}

sync

# Unmount and Detach the img
echo "mkflashableimg: Unmount and Detach the img"
umount /mnt/BOOT
umount /mnt/rootfs
rm -rf /mnt/BOOT /mnt/rootfs
losetup --detach-all

# mkflashableimg: Verify
echo "mkflashableimg: Verify"
parted ${OUT_IMG} unit MiB print

echo "Done."
