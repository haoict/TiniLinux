#!/bin/sh

source /root/partition-info.sh

echo "Extending rootfs partition by 100%" >> /dev/tty1

# Use parted to extend partition 2
echo -e "Yes\n" | parted ---pretend-input-tty ${MMC_DEV_FILE} -- unit s resizepart 2 -1s >> /dev/tty1 2>&1
if [ $? -ne 0 ]; then
    echo "Failed to extend rootfs partition" >> /dev/tty1
    exit 1
fi

sleep 1
partprobe ${MMC_DEV_FILE} >> /dev/tty1 2>&1

# Resize the filesystem
resize2fs ${ROOTFS_PART_DEV_FILE} >> /dev/tty1 2>&1
if [ $? -ne 0 ]; then
    echo "Failed to resize rootfs filesystem" >> /dev/tty1
    exit 1
fi

mv /root/.resize-me /root/.resize-me-done 2>/dev/null || true
mv /root/firstboot.sh /root/.firstboot.sh 2>/dev/null || true
echo "Rootfs extension completed" >> /dev/tty1
