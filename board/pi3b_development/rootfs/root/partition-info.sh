BOOT_SIZE=60 # minimum F32 size is 32MiB
ROOTFS_INIT_SIZE=600 # the initial rootfs size to make flashable image (MiB)
ROOTFS_TO_EXTEND_SIZE=-1 # the extra size to extend rootfs on first boot (-1 means use all remaining space)
BOOT_PART_START=1 # default by buildroot raspberry config (in sectors)
