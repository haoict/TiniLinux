#!/bin/bash

set -eu

mountpoint_microsd=/roms2
mountpoint_usb=/media/usb0
state_dir=/run/external-media-mount

usage() {
    echo "Usage: $0 [--unmount] mmcblkN" >&2
    exit 2
}

is_mounted() {
    awk -v device="$1" '$1 == device { found = 1 } END { exit !found }' /proc/mounts
}

unmount() {
    device=$1
    state_file="$state_dir/$device"
    case "$device" in
        mmcblk[0-9]*) mountpoint="$mountpoint_microsd" ;;
        sd[a-z]*[0-9]*) mountpoint="$mountpoint_usb" ;;
        *) usage ;;
    esac

    # Only unmount a filesystem that this service mounted.  This protects a
    # manually mounted filesystem at the same mountpoint.
    [ -e "$state_file" ] || exit 0
    umount "$mountpoint" || exit 0
    rm -f "$state_file"
}

# get kernel cmdline "bootpart" parameter (usually /dev/mmcblk0p1, or /dev/vda1, /dev/sda1), cut p1 to get the device file (e.g. /dev/mmcblk0)
DISK_DEVICE=$(cat /proc/cmdline | sed -n 's/.*bootpart=\([^ ]*\).*/\1/p' | sed 's/p1//' | sed 's/da1/da/')
if [ -z "$DISK_DEVICE" ]; then
    echo "Failed to get disk device file from kernel cmdline, did you set the 'bootpart' parameter correctly?"
    exit 1
fi


[ "$#" -ge 1 ] || usage

case "$1" in
    --unmount)
        [ "$#" = 2 ] || usage
        unmount "$2"
        ;;
    *)
        [ "$#" = 1 ] || usage
        device=$1
        # if device is the same as the boot device, do not mount it
        if [ "$device" = "${DISK_DEVICE#/dev/}" ]; then
            echo "Refusing to mount the boot device ($device)" >&2
            exit 0
        fi
        case "$device" in
            mmcblk[0-9]*) mountpoint="$mountpoint_microsd" ;;
            sd[a-z]*[0-9]*) mountpoint="$mountpoint_usb" ;;
            *) usage ;;
        esac

        # The disk uevent may arrive before partition nodes are created.  Wait
        # briefly for them, then support a card formatted without a partition
        # table as a fallback.
        mkdir -p "$mountpoint" "$state_dir"
        attempt=0
        while [ "$attempt" -lt 5 ]; do
            for candidate in /dev/"$device"p[0-9]*; do
                [ -b "$candidate" ] || continue
                is_mounted "$candidate" && continue
                if mount "$candidate" "$mountpoint"; then
                    : > "$state_dir/$device"
                    exit 0
                fi
            done
            attempt=$((attempt + 1))
            sleep 1
        done

        if [ -b "/dev/$device" ] && ! is_mounted "/dev/$device"; then
            if mount "/dev/$device" "$mountpoint"; then
                : > "$state_dir/$device"
                exit 0
            fi
        fi

        rmdir "$mountpoint" 2>/dev/null || true
        exit 1
        ;;
esac
