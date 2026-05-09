#!/bin/bash
mib_to_sectors() { echo $(( $1 * 1024 * 1024 / 512 )); } # sector size = 512 bytes

DISK_START_PADDING=$(( (BOOT_PART_START + 2047) / 2048 )) # Align to 1MiB (2048 sectors)
DISK_SIZE=$(( DISK_START_PADDING + BOOT_SIZE + ROOTFS_INIT_SIZE + ${QEMU_EXTRA_DISK_PADDING:-0} + 1 ))

BOOT_PART_END=$(( BOOT_PART_START + $(mib_to_sectors $BOOT_SIZE) - 1 ))
ROOTFS_PART_START=$(( BOOT_PART_END + 1 ))
ROOTFS_PART_INIT_END=$(( ROOTFS_PART_START + $(mib_to_sectors $ROOTFS_INIT_SIZE) - 1 ))
ROMS_PART_START=$(( ROOTFS_PART_INIT_END + $(mib_to_sectors $ROOTFS_TO_EXTEND_SIZE) + 1 ))