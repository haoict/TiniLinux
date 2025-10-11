# Partitions size, in MB
BOOT_SIZE=50
ROOTFS_SIZE=1024
# Sector size is 512 bytes & Default boot partition offset, in sectors (don't change this, it is required by u-boot)
SECTOR_SIZE=512
BOOT_PART_START=32768

DISK_START_PADDING=$(((${BOOT_PART_START} + 2048 - 1) / 2048))
DISK_GPT_PADDING=1
DISK_SIZE=$((${DISK_START_PADDING} + ${BOOT_SIZE} + ${ROOTFS_SIZE} + ${DISK_GPT_PADDING}))

BOOT_PART_END=$((${BOOT_PART_START} + (${BOOT_SIZE} * 1024 * 1024 / 512) - 1))
ROOTFS_PART_START=$((${BOOT_PART_END} + 1))
ROOTFS_PART_END=$((${ROOTFS_PART_START} + (${ROOTFS_SIZE} * 1024 * 1024 / 512) - 1))
