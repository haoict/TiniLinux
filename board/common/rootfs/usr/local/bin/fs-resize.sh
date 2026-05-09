#!/bin/bash

source /root/partition-info.sh
source /usr/local/bin/partition-info-helper.sh

# Accept filesystem type as parameter
FS_TYPE="$1"
# get kernel cmdline "bootpart" parameter (usually /dev/mmcblk0p1, or /dev/vda1, /dev/sda1), cut p1 to get the device file (e.g. /dev/mmcblk0)
DISK_DEVICE=$(cat /proc/cmdline | sed -n 's/.*bootpart=\([^ ]*\).*/\1/p' | sed 's/p1//' | sed 's/da1/da/')
if [ -z "$DISK_DEVICE" ]; then
    echo "Failed to get disk device file from kernel cmdline, did you set the 'bootpart' parameter correctly?"
    exit 1
fi

# if DISK_DEVICE contains "mmcblk", the partition suffix is "p", otherwise it's empty (sda -> sda2, mmcblk0 -> mmcblk0p2)
if [[ "$DISK_DEVICE" == *"mmcblk"* ]]; then
    ROOTFS_PART="${DISK_DEVICE}p2"
    ROMS_PART="${DISK_DEVICE}p3"
else
    ROOTFS_PART="${DISK_DEVICE}2"
    ROMS_PART="${DISK_DEVICE}3"
fi

if [ -z "$FS_TYPE" ]; then
    echo "Usage: $0 <filesystem-type>"
    echo "Filesystem types: rootfs, romsfs, rootfs_romsfs (for both at the same time)"
    exit 1
fi

if [ "$FS_TYPE" = "rootfs" ] || [ "$FS_TYPE" = "rootfs_romsfs" ]; then
    echo "Extending rootfs partition by $ROOTFS_TO_EXTEND_SIZE MB..."

    # Use parted to extend partition 2
    # if ROOTFS_TO_EXTEND_SIZE is -1, use all remaining space
    if [ "$ROOTFS_TO_EXTEND_SIZE" = "-1" ]; then
        echo -e "Yes\n" | parted ---pretend-input-tty $DISK_DEVICE unit s resizepart 2 100%
        if [ $? -ne 0 ]; then
            echo "Failed to extend rootfs partition"
            exit 1
        fi
    else
        echo -e "Yes\n" | parted ---pretend-input-tty $DISK_DEVICE unit s resizepart 2 $(($ROOTFS_PART_INIT_END + ($ROOTFS_TO_EXTEND_SIZE * 1024 * 1024 / 512)))
        if [ $? -ne 0 ]; then
            echo "Failed to extend rootfs partition"
            exit 1
        fi
    fi

    sleep 1
    partprobe $DISK_DEVICE

    # Resize the filesystem
    e2fsck -f $ROOTFS_PART
    resize2fs $ROOTFS_PART
    if [ $? -ne 0 ]; then
        echo "Failed to resize rootfs filesystem"
        exit 1
    fi

    echo "Rootfs extension completed."
    echo ""
    need_reboot=1
fi

if [ "$FS_TYPE" = "romsfs" ] || [ "$FS_TYPE" = "rootfs_romsfs" ]; then
    if [ ! -e $ROMS_PART ]; then
        # romfs partition doesn't exist, create a new partition using the rest of the disk
        echo "Creating ROMS partition starting at sector $ROMS_PART_START..."
        echo -e "n\np\n3\n$ROMS_PART_START\n\nw\n" | fdisk $DISK_DEVICE
        sleep 3

        # Changes the partition type of partition 3 on $DISK_DEVICE to type 7 (NTFS/exFAT/HPFS)
        echo -e "t\n3\n7\nw\n" | fdisk $DISK_DEVICE
        # sleep 3

        # Refreshes the partition table information of the device $DISK_DEVICE
        # partprobe $DISK_DEVICE

        echo "Creating $ROMS_PART done."
        echo ""
        need_reboot=1
    else # romsfs partition exists, format and populate it
        # check if romsfs already mounted, just exit
        if grep -qs "$ROMS_PART" /proc/mounts;
            then echo "$ROMS_PART already created and mounted. Exiting...";
            # Cleanup
            mv /root/.resize-romsfs /root/.resize-romsfs-done
            exit 0;
        fi

        # Format it as exfat
        mkfs.exfat -n ROMS $ROMS_PART
        if [ $? -ne 0 ]; then
            echo "mkfs.exfat $ROMS_PART failed"
            exit 1
        fi
        sleep 5

        # Mount exfat partition to /roms
        rm -rf /roms && mkdir -p /roms
        # echo "$ROMS_PART /roms exfat umask=0000,iocharset=utf8,noatime,nofail 0 0" >> /etc/fstab
        # systemctl daemon-reload
        mount -a
        mount | grep /roms

        # Popluating /roms
        tar -Jxvf /root/roms.tar.xz -C /roms --no-same-owner -m && echo "Extracting roms done" && rm /root/roms.tar.xz

        # Cleanup
        mv /root/.resize-romsfs /root/.resize-romsfs-done

        echo "Formatting $ROMS_PART done."
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
    exec systemctl default
fi
