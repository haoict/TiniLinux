# Partitions size, in MiB
BOOT_SIZE=33 # minimum F32 size is 32MiB, we set to 33 to avoid mkfs.fat errors
ROOTFS_INIT_SIZE=500 # the initial rootfs size to make flashable image
ROOTFS_TO_EXTEND_SIZE=2048 # the extra size to extend rootfs on first boot
# Sector size is 512 bytes & Default boot partition offset, in sectors (don't change this, it is required by u-boot)
SECTOR_SIZE=512
BOOT_PART_START=10

DISK_START_PADDING=$(((${BOOT_PART_START} + 2048 - 1) / 2048))
ROMS_ZISE=200
DISK_GPT_PADDING=$((${ROOTFS_TO_EXTEND_SIZE}+${ROMS_ZISE}+1))
DISK_SIZE=$((${DISK_START_PADDING} + ${BOOT_SIZE} + ${ROOTFS_INIT_SIZE} + ${DISK_GPT_PADDING}))

BOOT_PART_END=$((${BOOT_PART_START} + (${BOOT_SIZE} * 1024 * 1024 / 512) - 1))
ROOTFS_PART_START=$((${BOOT_PART_END} + 1))
ROOTFS_PART_INIT_END=$((${ROOTFS_PART_START} + (${ROOTFS_INIT_SIZE} * 1024 * 1024 / 512) - 1))
ROMS_PART_START=$((${ROOTFS_PART_INIT_END} + (${ROOTFS_TO_EXTEND_SIZE} * 1024 * 1024 / 512) + 1))

# Device files
MMC_DEV_FILE="/dev/vda"
ROOTFS_PART_DEV_FILE=$MMC_DEV_FILE"2"
ROMS_PART_DEV_FILE=$MMC_DEV_FILE"3"