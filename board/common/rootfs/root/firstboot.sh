#!/bin/sh

source /root/partition-info.sh

# Check if rootfs has already been extended
if [ -e /root/.resize-me ]; then
    echo "Extending rootfs partition by ${ROOTFS_TO_EXTEND_SIZE}MB..." >> /dev/tty1

    # Use parted to extend partition 2
    echo -e "Yes\n" | parted ---pretend-input-tty ${MMC_DEV_FILE} unit s resizepart 2 $((${ROOTFS_PART_INIT_END} + (${ROOTFS_TO_EXTEND_SIZE} * 1024 * 1024 / 512))) >> /dev/tty1 2>&1
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
    echo "Rootfs extension completed" >> /dev/tty1
fi

# Create, format and populate ROMS partition
if [ -e ${ROMS_PART_DEV_FILE} ]; then
    # ${ROMS_PART_DEV_FILE} already created.
    if grep -qs "${ROMS_PART_DEV_FILE}" /proc/mounts;
        then echo "${ROMS_PART_DEV_FILE} already created and mounted. Exiting...";
        # Cleanup
        mv /root/firstboot.sh /root/.firstboot-done.sh
        mv /root/partition-info.sh /root/.partition-info.sh
        exit 0;
    fi

    # Format it as exfat
    mkfs.exfat -n ROMS ${ROMS_PART_DEV_FILE} >> /dev/tty1 2>&1
    if [ $? -ne 0 ]; then
        echo "mkfs.exfat ${ROMS_PART_DEV_FILE} failed" >> /dev/tty1
        exit 1
    fi
    sleep 5

    # Mount exfat partition to /roms
    rm -rf /roms && mkdir -p /roms
    echo "${ROMS_PART_DEV_FILE} /roms exfat umask=0000,iocharset=utf8,noatime 0 0" >> /etc/fstab
    systemctl daemon-reload
    mount -a
    mount | grep /roms >/dev/tty1

    # Popluating /roms
    tar -Jxvf /root/roms.tar.xz -C /roms --no-same-owner >/dev/tty1 2>&1 && echo "Extracting roms done" >/dev/tty1 && rm /root/roms.tar.xz

    # Cleanup
    mv /root/firstboot.sh /root/.firstboot-done.sh
    mv /root/partition-info.sh /root/.partition-info.sh

    echo "Formatting ${ROMS_PART_DEV_FILE} done." >> /dev/tty1
    sleep 3
else
    # Create a new primary partition 3 on ${MMC_DEV_FILE} starting after the extended rootfs and using the rest of the disk
    echo "Creating ROMS partition starting at sector ${ROMS_PART_START}..." >> /dev/tty1
    echo -e "n\np\n3\n${ROMS_PART_START}\n\nw\n" | fdisk ${MMC_DEV_FILE} >> /dev/tty1 2>&1
    sleep 3

    # Changes the partition type of partition 3 on ${MMC_DEV_FILE} to type 7 (NTFS/exFAT/HPFS)
    echo -e "t\n3\n7\nw\n" | fdisk ${MMC_DEV_FILE} >> /dev/tty1 2>&1
    # sleep 3

    # Refreshes the partition table information of the device ${MMC_DEV_FILE}
    # partprobe ${MMC_DEV_FILE} >> /dev/tty1 2>&1

    echo "Creating ${ROMS_PART_DEV_FILE} done. Rebooting..." >> /dev/tty1
    sleep 3

    systemctl reboot -f
fi
