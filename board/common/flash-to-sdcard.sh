#!/bin/bash

set -e

echo "=========================================="
echo "  TiniLinux Flash to SD Card"
echo "=========================================="
echo ""

# Find the TiniLinux root directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
TINILINUX_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"

# Determine the image file path
IMG_FILE="$TINILINUX_ROOT/output.$BOARD/images/tinilinux-$BOARD.img"

if [ ! -f "$IMG_FILE" ]; then
    echo "  ✗ Error: Image file not found: $IMG_FILE"
    echo "  Please build the image first with 'make && make img'"
    exit 1
fi

echo "[1/3] Preparing..."
echo "  ✓ Board: $BOARD"
echo "  ✓ Image: $IMG_FILE"
echo "  ✓ Size: $(du -h $IMG_FILE | cut -f1)"
echo ""

echo "Available block devices:"
lsblk -d -o NAME,SIZE,TYPE,VENDOR,MODEL | grep -E "disk|NAME"
echo ""

# Try to auto-detect SD card device
AUTO_DEVICE=$(lsblk -d -n -o NAME,VENDOR,MODEL | grep -iE "(Mass|Storage Device)" | awk '{print $1}' | head -n 1)

if [ -n "$AUTO_DEVICE" ]; then
    # Auto-detected device
    DEVICE_INFO=$(lsblk -d -o NAME,SIZE,VENDOR,MODEL | grep "^$AUTO_DEVICE")
    echo "  ✓ Auto-detected SD card: /dev/$AUTO_DEVICE"
    echo "    $DEVICE_INFO"
    echo ""
    read -p "Use this device? (y/n or enter device name): " USER_INPUT
    
    if [ "$USER_INPUT" = "y" ]; then
        DEVICE="$AUTO_DEVICE"
    elif [ -n "$USER_INPUT" ] && [ "$USER_INPUT" != "n" ]; then
        # User entered a device name
        DEVICE="$USER_INPUT"
    else
        echo "  Aborted."
        exit 0
    fi
else
    # No auto-detection, ask for manual input
    read -p "Enter the SD card device (e.g., sdb, mmcblk0): " DEVICE
    
    if [ -z "$DEVICE" ]; then
        echo "  ✗ Error: No device specified"
        exit 1
    fi
fi

# Remove /dev/ prefix if user included it
DEVICE=${DEVICE#/dev/}

if [ ! -e "/dev/$DEVICE" ]; then
    echo "  ✗ Error: Device /dev/$DEVICE not found"
    exit 1
fi

# Final confirmation if not already confirmed
if [ -z "$AUTO_DEVICE" ] || [ "$USER_INPUT" != "y" ]; then
    echo ""
    echo "WARNING: This will erase all data on /dev/$DEVICE"
    echo ""
    read -p "Are you sure you want to continue? (y/n): " CONFIRM
    
    if [ "$CONFIRM" != "y" ]; then
        echo "  Aborted."
        exit 0
    fi
fi

echo ""
echo "[2/3] Flashing..."
echo "  ✓ Unmounting partitions"
sudo umount /dev/${DEVICE}* 2>/dev/null || true

echo "  ✓ Writing image to /dev/$DEVICE"
sudo dd if="$IMG_FILE" of="/dev/$DEVICE" bs=4M conv=fsync iflag=fullblock oflag=direct status=progress 2>&1 | cat

echo ""
echo "[3/3] Finalizing..."
echo "  ✓ Syncing"
sudo sync

echo "  ✓ Powering off SD card"
sudo udisksctl power-off -b "/dev/$DEVICE" 2>/dev/null || true

echo ""
echo "=========================================="
echo "  ✓ Flash complete!"
echo "  You can now remove the SD card safely."
echo "=========================================="
