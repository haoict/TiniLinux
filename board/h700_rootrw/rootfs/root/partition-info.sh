BOOT_SIZE=33 # minimum F32 size is 32MiB
ROOTFS_INIT_SIZE=300 # the initial rootfs size to make flashable image (MiB)
ROOTFS_TO_EXTEND_SIZE=500 # the extra size to extend rootfs on first boot (MiB)
BOOT_PART_START=32768 # default boot partition offset (in sectors) (don't change this, it is required by u-boot)
