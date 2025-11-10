# Partitions size, in MiB
BOOT_SIZE=30
ROOTFS_INIT_SIZE=300 # the initial rootfs size to make flashable image
ROOTFS_TO_EXTEND_SIZE=500 # the extra size to extend rootfs on first boot
# Sector size is 512 bytes & Default boot partition offset, in sectors (don't change this, it is required by u-boot)
SECTOR_SIZE=512
BOOT_PART_START=32768

DISK_START_PADDING=$(((${BOOT_PART_START} + 2048 - 1) / 2048))
DISK_GPT_PADDING=1
DISK_SIZE=$((${DISK_START_PADDING} + ${BOOT_SIZE} + ${ROOTFS_INIT_SIZE} + ${DISK_GPT_PADDING}))

BOOT_PART_END=$((${BOOT_PART_START} + (${BOOT_SIZE} * 1024 * 1024 / 512) - 1))
ROOTFS_PART_START=$((${BOOT_PART_END} + 1))
ROOTFS_PART_INIT_END=$((${ROOTFS_PART_START} + (${ROOTFS_INIT_SIZE} * 1024 * 1024 / 512) - 1))
ROMS_PART_START=$((${ROOTFS_PART_INIT_END} + (${ROOTFS_TO_EXTEND_SIZE} * 1024 * 1024 / 512) + 1))

# Device files
MMC_DEV_FILE="/dev/mmcblk0"
ROOTFS_PART_DEV_FILE=$MMC_DEV_FILE"p2"
ROMS_PART_DEV_FILE=$MMC_DEV_FILE"p3"