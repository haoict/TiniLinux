#!/bin/bash
# Ref about commands: https://raspberrypi.stackexchange.com/questions/78466/how-to-make-an-image-file-from-scratch/78467#78467
# Check if user is root
if [[ $EUID -ne 0 ]]; then
    echo "This script must be run as root." 
    exit 1
fi

set -euo pipefail

echo "======================================================="
echo " Creating Flashable Image for ${BOARD} (RootRW)"
echo " (Loop device mount variant, need host root privileges)"
echo "======================================================="
echo ""

# Load partition info variables
source ${BR2_EXTERNAL_TiniLinux_PATH}/board/${BOARD}/rootfs/root/partition-info.sh

OUT_IMG=images/tinilinux-${BOARD}.img
rm -f ${OUT_IMG}

echo "[1/5] Setting up disk image (${DISK_SIZE}M)..."
echo "  ✓ Creating empty disk image"
truncate -s ${DISK_SIZE}M ${OUT_IMG}

echo "  ✓ Initializing MBR partition table"
parted ${OUT_IMG} mktable msdos

if [[ "${BOARD}" == "rgb30"* ]]; then
    echo "  ✓ Writing U-Boot bootloader (offset: 32KiB)"
    dd if=images/u-boot-rockchip.bin of=${OUT_IMG} bs=512 seek=64 conv=fsync,notrunc
elif [[ "${BOARD}" == "h700"* ]]; then
    echo "  ✓ Writing U-Boot bootloader (offset: 8KiB)"
    dd if=images/u-boot-sunxi-with-spl.bin of=${OUT_IMG} bs=1K seek=8 conv=fsync,notrunc
elif [[ "${BOARD}" == *"qemu"* ]]  || [[ "${BOARD}" == *"pi3b"* ]]; then
    echo "  ✓ Skipping U-Boot for board ${BOARD}"
else
    echo "  ✗ Error: U-Boot not implemented for board ${BOARD}"
    exit 1
fi

echo ""
echo "[2/5] Creating partitions..."
echo "  ✓ Creating BOOT partition (${BOOT_SIZE}M, FAT32)"
parted -s ${OUT_IMG} -a min unit s mkpart primary fat32 ${BOOT_PART_START} ${BOOT_PART_END}

echo "  ✓ Creating rootfs partition (${ROOTFS_INIT_SIZE}M, ext4)"
parted -s ${OUT_IMG} -a min unit s mkpart primary ext4 ${ROOTFS_PART_START} ${ROOTFS_PART_INIT_END}

echo "  ✓ Setting boot flag"
parted -s ${OUT_IMG} set 1 boot on
sync

echo ""
echo "[3/5] Setting up loop device..."
echo "  ✓ Cleaning up previous mounts"
umount /mnt/BOOT || true
umount /mnt/rootfs || true
rm -rf /mnt/BOOT /mnt/rootfs
losetup --detach-all || true

echo "  ✓ Creating loop device"
DEV_LOOP=$(losetup --show --find --partscan ${OUT_IMG})
sync
echo "  ✓ Formatting BOOT partition (FAT32)"
mkfs.fat -F32 -n BOOT ${DEV_LOOP}p1
echo "  ✓ Formatting rootfs partition (ext4)"
mkfs.ext4 -O ^orphan_file -L rootfs ${DEV_LOOP}p2

echo "  ✓ Mounting partitions"
mkdir -p /mnt/BOOT && mount -t vfat ${DEV_LOOP}p1 /mnt/BOOT
mkdir -p /mnt/rootfs && mount -t ext4 ${DEV_LOOP}p2 /mnt/rootfs

echo ""
echo "[4/5] Copying files..."
echo "  ✓ Copying boot files"
cp images/Image /mnt/BOOT/
cp images/initramfs /mnt/BOOT/
if [[ "${BOARD}" == "rgb30"* ]]; then
    cp -r images/rockchip /mnt/BOOT/dtb
    cp images/rk3566-dtbo/*.dtbo /mnt/BOOT/dtb/
elif [[ "${BOARD}" == "h700"* ]]; then
    cp -r images/allwinner /mnt/BOOT/dtb
elif [[ "${BOARD}" == *"pi3b"* ]]; then
    cp -r images/rpi-firmware/* /mnt/BOOT/
    cp -r images/broadcom/* /mnt/BOOT/
fi
cp -r ${BR2_EXTERNAL_TiniLinux_PATH}/board/${BOARD}/BOOT/* /mnt/BOOT/

echo "  ✓ Extracting rootfs"
tar -xf images/rootfs.tar -C /mnt/rootfs --no-same-owner
echo "  ✓ Updating build info"
sed -i "s/^BUILD_ID=buildroot/BUILD_ID=$(TZ='Asia/Tokyo' date +%Y%m%d-%H%M)JST/" /mnt/rootfs/etc/os-release
echo "  ✓ Preparing ROMs archive"
romtmp=$(mktemp -d)
cp -r ${BR2_EXTERNAL_TiniLinux_PATH}/board/common/ROMS/ ${romtmp}/
if [ -d ${BR2_EXTERNAL_TiniLinux_PATH}/board/${BOARD}/ROMS/ ]; then cp -r ${BR2_EXTERNAL_TiniLinux_PATH}/board/${BOARD}/ROMS/ ${romtmp}/; fi
if [ -d ${BR2_EXTERNAL_TiniLinux_PATH}/board/common/private-ROMS/ ]; then cp -r ${BR2_EXTERNAL_TiniLinux_PATH}/board/common/private-ROMS/* ${romtmp}/ROMS/; fi
tar -Jcf /mnt/rootfs/root/roms.tar.xz -C ${romtmp}/ROMS/ .
rm -rf ${romtmp}

sync

echo ""
echo "[5/5] Finalizing..."
echo "  ✓ Unmounting partitions"
umount /mnt/BOOT
umount /mnt/rootfs
rm -rf /mnt/BOOT /mnt/rootfs
echo "  ✓ Detaching loop device"
losetup --detach-all

parted ${OUT_IMG} unit MiB print

# if "ZIP=0 make img" then do not create zip archive
if [[ "${ZIP:-1}" != "0" ]]; then
    echo "  ✓ Creating zip archive"
    zip -j ${OUT_IMG}.zip ${OUT_IMG}
    rm -f ${OUT_IMG}
    OUT_IMG=${OUT_IMG}.zip
fi

echo ""
echo "=========================================="
echo "  ✓ Image created successfully!"
echo "  Location: ${OUT_IMG}"
echo "  Size: $(du -h ${OUT_IMG} | cut -f1)"
echo "=========================================="
