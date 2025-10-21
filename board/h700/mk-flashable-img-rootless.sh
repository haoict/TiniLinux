#!/bin/bash
set -euo pipefail

#####################################################
# SETUP ENV
#####################################################

BOARD=h700
# Load partition info variables
source board/${BOARD}/rootfs/root/partition-info.sh
OUT_IMG=output.${BOARD}/images/tinilinux-${BOARD}.img
rm -f ${OUT_IMG}


#####################################################
# CREATE SKELEON OUPUT DISK IMAGE (PARTITIONING)
#####################################################

# mkflashableimg: Create an empty img file
echo "mkflashableimg: Create an empty img file"
truncate -s ${DISK_SIZE}M ${OUT_IMG}

# mkflashableimg: Make the disk MBR type (msdos)
echo "mkflashableimg: Make the disk MBR type (msdos)"
parted ${OUT_IMG} mktable msdos

# mkflashableimg: Write the u-boot to the img (offset 16 sectors = 8KiB)
echo "mkflashableimg: Write the u-boot to the img (offset 16 sectors = 8KiB)"
dd if=output.${BOARD}/build/uboot-2025.07/u-boot-sunxi-with-spl.bin of=${OUT_IMG} bs=1K seek=8 conv=fsync,notrunc

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


#####################################################
# FORMAT PARTITIONS AND COPY DATA
#####################################################

# mkflashableimg: Create part1 (BOOT partition)
P1_IMG=output.${BOARD}/images/p1.img
rm -f ${P1_IMG}
truncate -s ${BOOT_SIZE}M ${P1_IMG}
mkfs.fat -F32 -n BOOT ${P1_IMG}
mcopy -i ${P1_IMG} -o board/${BOARD}/BOOT/* ::/
mcopy -i ${P1_IMG} -o output.${BOARD}/images/Image ::/
mcopy -i ${P1_IMG} -o output.${BOARD}/images/initramfs ::/
mcopy -i ${P1_IMG} -o output.${BOARD}/images/allwinner/ ::/dtb
# mkflashableimg: Verify part1
mdir -i output.${BOARD}/images/p1.img ::/
sync
fsck.fat -n ${P1_IMG}
# mkflashableimg: Merge part1 to output img
dd if=${P1_IMG} of="${OUT_IMG}" bs=512 seek="${BOOT_PART_START}" conv=fsync,notrunc
rm -f ${P1_IMG}

# mkflashableimg: Create part2 (rootfs partition)
P2_IMG=output.${BOARD}/images/p2.img
rm -f ${P2_IMG}
truncate -s ${ROOTFS_SIZE}M ${P2_IMG}
mkfs.ext4 -O ^orphan_file -L rootfs ${P2_IMG}
rootfstmp=$(mktemp -d)
tar -xf output.${BOARD}/images/rootfs.tar -C $rootfstmp
# Create roms.tar.xz to /root to be used in firstboot
romtmp=$(mktemp -d)
cp -r board/common/ROMS/ ${romtmp}/
if [ -d board/${BOARD}/ROMS/ ]; then cp -r board/${BOARD}/ROMS/ ${romtmp}/; fi
tar -Jcf $rootfstmp/roms.tar.xz -C ${romtmp}/ROMS/ .
rm -rf ${romtmp}
if [[ "$(uname -m)" == "x86_64" ]]; then
    ./board/common/populatefs-amd64 -U -d $rootfstmp ${P2_IMG}
elif [[ "$(uname -m)" == "aarch64" || "$(uname -m)" == "arm64" ]]; then
    ./board/common/populatefs-arm64 -U -d $rootfstmp ${P2_IMG}
fi
sync
e2fsck -n ${P2_IMG}
rm -rf ${rootfstmp}
# mkflashableimg: Merge part12 to output img
dd if=${P2_IMG} of="${OUT_IMG}" bs=512 seek="${ROOTFS_PART_START}" conv=fsync,notrunc
rm -f ${P2_IMG}

# mkflashableimg: Verify
echo "mkflashableimg: Verify"
parted ${OUT_IMG} unit MiB print

echo "Done."
