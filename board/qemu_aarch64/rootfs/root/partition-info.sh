BOOT_SIZE=100 # minimum F32 size is 32MiB
ROOTFS_INIT_SIZE=50 # the initial rootfs size to make flashable image (MiB)
ROOTFS_TO_EXTEND_SIZE=300 # the extra size to extend rootfs on first boot (MiB)
BOOT_PART_START=10 # (in sectors)
QEMU_EXTRA_DISK_PADDING=$(( ROOTFS_TO_EXTEND_SIZE + 200 )) # ROMS size is 200MiB