#!/bin/bash
set -euo pipefail

#####################################################
# SETUP ENV
#####################################################

echo "=========================================="
echo "  Creating Flashable Image for ${BOARD}"
echo "=========================================="
echo ""

# Load partition info variables
source board/${BOARD}/rootfs/root/partition-info.sh
OUT_IMG=output.${BOARD}/images/tinilinux-${BOARD}.img
rm -f ${OUT_IMG}

echo "[1/5] Setting up disk image (${DISK_SIZE}M)..."
echo "  ✓ Creating empty disk image"
truncate -s ${DISK_SIZE}M ${OUT_IMG}

echo "  ✓ Initializing MBR partition table"
parted ${OUT_IMG} mktable msdos

if [[ "${BOARD}" == "rgb30"* ]]; then
    echo "  ✓ Writing U-Boot bootloader (offset: 32KiB)"
    dd if=output.${BOARD}/images/u-boot-rockchip.bin of=${OUT_IMG} bs=512 seek=64 conv=fsync,notrunc
elif [[ "${BOARD}" == "h700"* ]]; then
    echo "  ✓ Writing U-Boot bootloader (offset: 8KiB)"
    dd if=output.${BOARD}/images/u-boot-sunxi-with-spl.bin of=${OUT_IMG} bs=1K seek=8 conv=fsync,notrunc
elif [[ "${BOARD}" == *"qemu"* ]]; then
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


echo "\n[3/5] Formatting BOOT partition..."
P1_IMG=output.${BOARD}/images/p1.img
rm -f ${P1_IMG}
truncate -s ${BOOT_SIZE}M ${P1_IMG}
echo "  ✓ Formatting as FAT32"
mkfs.fat -F32 -n BOOT ${P1_IMG}
echo "  ✓ Copying boot files"
mcopy -i ${P1_IMG} -o board/${BOARD}/BOOT/* ::/
mcopy -i ${P1_IMG} -o output.${BOARD}/images/Image ::/
mcopy -i ${P1_IMG} -o output.${BOARD}/images/initramfs ::/
if [[ "${BOARD}" == "rgb30"* ]]; then
    mcopy -i ${P1_IMG} -o output.${BOARD}/images/rockchip/ ::/dtb
    mcopy -i ${P1_IMG} -o output.${BOARD}/images/rk3566-dtbo/*.dtbo ::/dtb
elif [[ "${BOARD}" == "h700"* ]]; then
    mcopy -i ${P1_IMG} -o output.${BOARD}/images/allwinner/ ::/dtb
fi
echo "  ✓ Verifying BOOT partition"
mdir -i output.${BOARD}/images/p1.img ::/
sync
fsck.fat -n ${P1_IMG}
echo "  ✓ Writing BOOT partition to image"
dd if=${P1_IMG} of="${OUT_IMG}" bs=512 seek="${BOOT_PART_START}" conv=fsync,notrunc
rm -f ${P1_IMG}

echo ""
echo "[4/5] Creating rootfs partition..."
P2_IMG=output.${BOARD}/images/p2.img
rm -f ${P2_IMG}
truncate -s ${ROOTFS_INIT_SIZE}M ${P2_IMG}
echo "  ✓ Formatting as ext4"
mkfs.ext4 -O ^orphan_file -L rootfs ${P2_IMG}
rootfstmp=$(mktemp -d)
echo "  ✓ Extracting rootfs"
tar -xf output.${BOARD}/images/rootfs.tar -C $rootfstmp
echo "  ✓ Updating build info"
sed -i "s/^BUILD_ID=buildroot/BUILD_ID=$(TZ='Asia/Tokyo' date +%Y%m%d-%H%M)JST/" $rootfstmp/etc/os-release
echo "  ✓ Preparing ROMs archive"
romtmp=$(mktemp -d)
cp -r board/common/ROMS/ ${romtmp}/
if [ -d board/${BOARD}/ROMS/ ]; then cp -r board/${BOARD}/ROMS/ ${romtmp}/; fi
tar -Jcf $rootfstmp/root/roms.tar.xz -C ${romtmp}/ROMS/ .
rm -rf ${romtmp}
echo "  ✓ Populating filesystem"
if [[ "$(uname -m)" == "x86_64" ]]; then
    ./board/common/populatefs-amd64 -U -d $rootfstmp ${P2_IMG}
elif [[ "$(uname -m)" == "aarch64" || "$(uname -m)" == "arm64" ]]; then
    ./board/common/populatefs-arm64 -U -d $rootfstmp ${P2_IMG}
fi
sync
echo "  ✓ Verifying rootfs"
e2fsck -n ${P2_IMG}
rm -rf ${rootfstmp}
echo "  ✓ Writing rootfs partition to image"
dd if=${P2_IMG} of="${OUT_IMG}" bs=512 seek="${ROOTFS_PART_START}" conv=fsync,notrunc
rm -f ${P2_IMG}

echo ""
echo "[5/5] Finalizing..."
parted ${OUT_IMG} unit MiB print

echo ""
echo "=========================================="
echo "  ✓ Image created successfully!"
echo "  Location: ${OUT_IMG}"
echo "  Size: $(du -h ${OUT_IMG} | cut -f1)"
echo "=========================================="
