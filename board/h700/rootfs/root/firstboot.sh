#!/bin/sh

# # Rebuild kernel module dependency and load rocknix-singleadc-joypad.ko
# depmod -a
# modprobe rocknix-singleadc-joypad

# Create, format and populate ROMS partition
if [ -e /dev/mmcblk0p3 ]; then
    # /dev/mmcblk0p3 already created.
    if grep -qs '/dev/mmcblk0p3' /proc/mounts;
        then echo "/dev/mmcblk0p3 already created and mounted. Exiting...";
        exit 0;
    fi

    # Format it as exfat
    mkfs.exfat -n ROMS /dev/mmcblk0p3 >> /dev/tty1 2>&1
    if [ $? -ne 0 ]; then
        echo "mkfs.exfat /dev/mmcblk0p3 failed" >> /dev/tty1
        exit 1
    fi
    sleep 5

    # Mount exfat partition to /roms
    rm -rf /roms && mkdir -p /roms
    echo "/dev/mmcblk0p3 /roms exfat umask=0000,iocharset=utf8,noatime 0 0" >> /etc/fstab
    systemctl daemon-reload
    mount -a
    mount | grep /roms

    # Popluating /roms
    tar -Jxvf /root/roms.tar.xz -C /roms --no-same-owner >> /dev/tty1 2>&1

    # Cleanup
    mv /root/firstboot.sh /root/.firstboot-done.sh
    mv /root/partition-info.sh /root/.partition-info.sh
    rm /root/roms.tar.xz

    echo "Formatting /dev/mmcblk0p3 done." >> /dev/tty1
    sleep 3
else
    # Create a new primary partition 3 on /dev/mmcblk0 starting at sector 2232320 and using the rest of the disk
    # equivalent parted command: parted /dev/mmcblk0 mkpart primary ext4 2232320s 100% >> /dev/tty1 2>&1
    source /root/partition-info.sh
    ROMS_PART_START=$((${ROOTFS_PART_END} + 1))
    echo -e "n\np\n3\n${ROMS_PART_START}\n\nw\n" | fdisk /dev/mmcblk0 >> /dev/tty1 2>&1
    sleep 3

    # Changes the partition type of partition 3 on /dev/mmcblk0 to type 7 (NTFS/exFAT/HPFS)
    echo -e "t\n3\n7\nw\n" | fdisk /dev/mmcblk0 >> /dev/tty1 2>&1
    sleep 3
    
    # Refreshes the partition table information of the device /dev/mmcblk0
    partprobe /dev/mmcblk0 >> /dev/tty1 2>&1

    echo "Creating /dev/mmcblk0p3 done. Rebooting..." >> /dev/tty1
    sleep 3

    systemctl reboot -f
fi



