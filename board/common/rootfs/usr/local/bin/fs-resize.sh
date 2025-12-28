#!/bin/sh

source /root/partition-info.sh

# Accept filesystem type as parameter
FS_TYPE="$1"

if [ -z "$FS_TYPE" ]; then
    echo "Usage: $0 <filesystem-type>"
    echo "Filesystem types: rootfs, romsfs, rootfs_romsfs (for both at the same time)"
    exit 1
fi

if [ "$FS_TYPE" = "rootfs" ] || [ "$FS_TYPE" = "rootfs_romsfs" ]; then
    echo "Extending rootfs partition by ${ROOTFS_TO_EXTEND_SIZE}MB..."

    # Use parted to extend partition 2
    echo -e "Yes\n" | parted ---pretend-input-tty ${MMC_DEV_FILE} unit s resizepart 2 $((${ROOTFS_PART_INIT_END} + (${ROOTFS_TO_EXTEND_SIZE} * 1024 * 1024 / 512)))
    if [ $? -ne 0 ]; then
        echo "Failed to extend rootfs partition"
        exit 1
    fi

    sleep 1
    partprobe ${MMC_DEV_FILE}

    # Resize the filesystem
    e2fsck -f ${ROOTFS_PART_DEV_FILE}
    resize2fs ${ROOTFS_PART_DEV_FILE}
    if [ $? -ne 0 ]; then
        echo "Failed to resize rootfs filesystem"
        exit 1
    fi

    echo "Rootfs extension completed."
    echo ""
    need_reboot=1
fi

if [ "$FS_TYPE" = "romsfs" ] || [ "$FS_TYPE" = "rootfs_romsfs" ]; then
    if [ ! -e ${ROMS_PART_DEV_FILE} ]; then
        # romfs partition doesn't exist, create a new partition using the rest of the disk
        echo "Creating ROMS partition starting at sector ${ROMS_PART_START}..."
        echo -e "n\np\n3\n${ROMS_PART_START}\n\nw\n" | fdisk ${MMC_DEV_FILE}
        sleep 3

        # Changes the partition type of partition 3 on ${MMC_DEV_FILE} to type 7 (NTFS/exFAT/HPFS)
        echo -e "t\n3\n7\nw\n" | fdisk ${MMC_DEV_FILE}
        # sleep 3

        # Refreshes the partition table information of the device ${MMC_DEV_FILE}
        # partprobe ${MMC_DEV_FILE}

        echo "Creating ${ROMS_PART_DEV_FILE} done."
        echo ""
        need_reboot=1
    else # romsfs partition exists, format and populate it
        # check if romsfs already mounted, just exit
        if grep -qs "${ROMS_PART_DEV_FILE}" /proc/mounts;
            then echo "${ROMS_PART_DEV_FILE} already created and mounted. Exiting...";
            # Cleanup
            mv /root/.resize-romsfs /root/.resize-romsfs-done
            exit 0;
        fi

        # Format it as exfat
        mkfs.exfat -n ROMS ${ROMS_PART_DEV_FILE}
        if [ $? -ne 0 ]; then
            echo "mkfs.exfat ${ROMS_PART_DEV_FILE} failed"
            exit 1
        fi
        sleep 5

        # Mount exfat partition to /roms
        rm -rf /roms && mkdir -p /roms
        echo "${ROMS_PART_DEV_FILE} /roms exfat umask=0000,iocharset=utf8,noatime,nofail 0 0" >> /etc/fstab
        systemctl daemon-reload
        mount -a
        mount | grep /roms

        # Popluating /roms
        tar -Jxvf /root/roms.tar.xz -C /roms --no-same-owner && echo "Extracting roms done" && rm /root/roms.tar.xz

        # Cleanup
        mv /root/.resize-romsfs /root/.resize-romsfs-done

        echo "Formatting ${ROMS_PART_DEV_FILE} done."
        echo ""
        need_reboot=0
    fi
fi

if [ "$need_reboot" = "1" ]; then
    echo "Rebooting..."
    sleep 3
    systemctl reboot -f
else
    echo "Continuing to normal boot..."
    exec systemctl isolate default.target
fi
