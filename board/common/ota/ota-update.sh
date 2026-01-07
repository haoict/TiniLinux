#!/bin/sh
#
# TiniLinux OTA Update Manager
# Downloads and installs OTA updates to the inactive slot
#

set -e

AB_SLOT_MGR="/usr/local/bin/ab-slot-mgr.sh"
BOOT_PARTITION="/dev/vda1"  # Will be overridden based on board
OVERLAY_PARTITION="/dev/vda2"
DOWNLOAD_DIR="/tmp/ota-download"
UPDATE_STATUS_FILE="/mnt/overlayfs/ota-status"

# Board-specific device detection
detect_boot_device() {
    if [ -b "/dev/mmcblk0p1" ]; then
        BOOT_PARTITION="/dev/mmcblk0p1"
        OVERLAY_PARTITION="/dev/mmcblk0p2"
    elif [ -b "/dev/mmcblk1p1" ]; then
        BOOT_PARTITION="/dev/mmcblk1p1"
        OVERLAY_PARTITION="/dev/mmcblk1p2"
    fi
}

# Download OTA package
download_update() {
    local url=$1
    local checksum=${2:-}
    
    echo "Downloading OTA update from: $url"
    mkdir -p "$DOWNLOAD_DIR"
    
    # Download the update package
    if command -v wget >/dev/null 2>&1; then
        wget -O "$DOWNLOAD_DIR/update.tar.gz" "$url" || {
            echo "Failed to download update"
            return 1
        }
    elif command -v curl >/dev/null 2>&1; then
        curl -L -o "$DOWNLOAD_DIR/update.tar.gz" "$url" || {
            echo "Failed to download update"
            return 1
        }
    else
        echo "Neither wget nor curl available"
        return 1
    fi
    
    # Verify checksum if provided
    if [ -n "$checksum" ]; then
        echo "Verifying checksum..."
        echo "$checksum  $DOWNLOAD_DIR/update.tar.gz" | sha256sum -c - || {
            echo "Checksum verification failed"
            rm -rf "$DOWNLOAD_DIR"
            return 1
        }
    fi
    
    echo "Download complete"
}

# Extract and verify update package
extract_update() {
    echo "Extracting update package..."
    cd "$DOWNLOAD_DIR"
    tar -xzf update.tar.gz || {
        echo "Failed to extract update"
        return 1
    }
    
    # Verify required files exist
    if [ ! -f "rootfs.squashfs" ]; then
        echo "Missing rootfs.squashfs in update package"
        return 1
    fi
    
    # Optional: verify kernel and initramfs
    # if [ ! -f "Image" ] || [ ! -f "initramfs" ]; then
    #     echo "Warning: Kernel or initramfs missing"
    # fi
    
    echo "Extraction complete"
}

# Install update to inactive slot
install_update() {
    detect_boot_device
    
    local inactive_slot=$($AB_SLOT_MGR get-inactive)
    echo "Installing update to slot: $inactive_slot"
    
    # Mount boot partition
    local boot_mount="/tmp/boot-mount"
    mkdir -p "$boot_mount"
    mount "$BOOT_PARTITION" "$boot_mount" || {
        echo "Failed to mount boot partition"
        return 1
    }
    
    # Copy new squashfs image
    echo "Installing rootfs.squashfs for slot $inactive_slot..."
    cp "$DOWNLOAD_DIR/rootfs.squashfs" "$boot_mount/rootfs-${inactive_slot}.squashfs" || {
        umount "$boot_mount"
        echo "Failed to copy rootfs"
        return 1
    }
    
    # Update kernel and initramfs if included
    if [ -f "$DOWNLOAD_DIR/Image" ]; then
        echo "Updating kernel..."
        cp "$DOWNLOAD_DIR/Image" "$boot_mount/Image-${inactive_slot}"
    fi
    
    if [ -f "$DOWNLOAD_DIR/initramfs" ]; then
        echo "Updating initramfs..."
        cp "$DOWNLOAD_DIR/initramfs" "$boot_mount/initramfs-${inactive_slot}"
    fi
    
    # Copy device trees if present
    if [ -d "$DOWNLOAD_DIR/dtb" ]; then
        echo "Updating device trees..."
        cp -r "$DOWNLOAD_DIR/dtb" "$boot_mount/dtb-${inactive_slot}/"
    fi
    
    sync
    umount "$boot_mount"
    rmdir "$boot_mount"
    
    echo "Update installed to slot $inactive_slot"
}

# Mark inactive slot as pending for next boot
activate_update() {
    local inactive_slot=$($AB_SLOT_MGR get-inactive)
    echo "Setting slot $inactive_slot as pending for next boot..."
    $AB_SLOT_MGR set-pending "$inactive_slot"
    
    echo ""
    echo "==================================="
    echo "OTA Update Ready"
    echo "==================================="
    echo "Slot $inactive_slot will be activated on next reboot."
    echo "If the new system fails to boot, it will automatically"
    echo "fall back to the current slot after 3 attempts."
    echo ""
    echo "To reboot now: systemctl reboot"
    echo "To cancel update: $AB_SLOT_MGR set-pending ''"
    echo "==================================="
}

# Full OTA update workflow
perform_ota() {
    local url=$1
    local checksum=${2:-}
    
    echo "==================================="
    echo "Starting OTA Update"
    echo "==================================="
    
    # Clean previous downloads
    rm -rf "$DOWNLOAD_DIR"
    
    # Download
    download_update "$url" "$checksum" || {
        echo "OTA update failed: download error"
        echo "failed" > "$UPDATE_STATUS_FILE"
        return 1
    }
    
    # Extract
    extract_update || {
        echo "OTA update failed: extraction error"
        echo "failed" > "$UPDATE_STATUS_FILE"
        rm -rf "$DOWNLOAD_DIR"
        return 1
    }
    
    # Install
    install_update || {
        echo "OTA update failed: installation error"
        echo "failed" > "$UPDATE_STATUS_FILE"
        rm -rf "$DOWNLOAD_DIR"
        return 1
    }
    
    # Activate
    activate_update
    
    # Cleanup
    rm -rf "$DOWNLOAD_DIR"
    echo "success" > "$UPDATE_STATUS_FILE"
    
    echo "OTA update completed successfully"
}

# Rollback to previous slot
rollback() {
    echo "Rolling back to previous slot..."
    local current=$($AB_SLOT_MGR get-active)
    local other=$($AB_SLOT_MGR get-inactive)
    
    $AB_SLOT_MGR set-pending "$other"
    echo "Rollback prepared. Reboot to activate slot $other"
}

# Show OTA status
show_status() {
    echo "=== OTA Update Status ==="
    if [ -f "$UPDATE_STATUS_FILE" ]; then
        echo "Last update: $(cat $UPDATE_STATUS_FILE)"
    else
        echo "No update history"
    fi
    echo ""
    $AB_SLOT_MGR status
}

# Main command dispatcher
case "${1:-}" in
    download)
        download_update "$2" "${3:-}"
        ;;
    extract)
        extract_update
        ;;
    install)
        install_update
        ;;
    activate)
        activate_update
        ;;
    update)
        perform_ota "$2" "${3:-}"
        ;;
    rollback)
        rollback
        ;;
    status)
        show_status
        ;;
    *)
        echo "TiniLinux OTA Update Manager"
        echo ""
        echo "Usage: $0 <command> [options]"
        echo ""
        echo "Commands:"
        echo "  download <url> [checksum]  - Download OTA package"
        echo "  extract                    - Extract downloaded package"
        echo "  install                    - Install to inactive slot"
        echo "  activate                   - Set inactive slot as pending"
        echo "  update <url> [checksum]    - Complete OTA workflow"
        echo "  rollback                   - Rollback to previous slot"
        echo "  status                     - Show OTA status"
        echo ""
        echo "Example:"
        echo "  $0 update http://example.com/update.tar.gz abc123..."
        exit 1
        ;;
esac
