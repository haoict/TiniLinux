#!/bin/bash
# Ref about commands: https://raspberrypi.stackexchange.com/questions/78466/how-to-make-an-image-file-from-scratch/78467#78467
# Check if user is root
if [[ $EUID -ne 0 ]]; then
   echo "This script must be run as root." 
   exit 1
fi

# Load partition info variables
source board/h700/rootfs/root/partition-info.sh

OUT_IMG=output.h700/images/tinilinux-h700.img

rm -f ${OUT_IMG}

# mkflashableimg: Create an empty img file
echo "mkflashableimg: Create an empty img file"
truncate -s ${DISK_SIZE}M ${OUT_IMG}

# mkflashableimg: Make the disk MBR type (msdos)
echo "mkflashableimg: Make the disk MBR type (msdos)"
parted ${OUT_IMG} mktable msdos

# mkflashableimg: Write the u-boot to the img (offset 16 sectors = 8KiB)
echo "mkflashableimg: Write the u-boot to the img (offset 16 sectors = 8KiB)"
dd if=output.h700/build/uboot-2025.07/u-boot-sunxi-with-spl.bin of=${OUT_IMG} bs=1K seek=8 conv=fsync,notrunc

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


# Format partitions and mount
echo "mkflashableimg: Format partitions and mount"
DEV_LOOP=$(losetup --show --find --partscan ${OUT_IMG})
mkfs.fat -F32 -n BOOT ${DEV_LOOP}p1
mkfs.ext4 -O ^orphan_file -L rootfs ${DEV_LOOP}p2

mkdir -p /mnt/BOOT && mount -t vfat ${DEV_LOOP}p1 /mnt/BOOT
mkdir -p /mnt/rootfs && mount -t ext4 ${DEV_LOOP}p2 /mnt/rootfs

# Copy kernel, initrd, dtb to /mnt/BOOT
echo "mkflashableimg: Copy kernel, initrd, dtb to /mnt/BOOT"
cp -r board/h700/BOOT/* /mnt/BOOT/
cp output.h700/images/Image /mnt/BOOT/

# Extract buildroot (output.h700/images/rootfs.tar) to /mnt/rootfs
echo "mkflashableimg: Extract buildroot (output.h700/images/rootfs.tar) to /mnt/rootfs"
tar -xf output.h700/images/rootfs.tar -C /mnt/rootfs --no-same-owner
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
