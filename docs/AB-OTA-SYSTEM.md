# TiniLinux A/B OTA Update System

## Overview

TiniLinux now supports A/B seamless over-the-air (OTA) updates with automatic fallback protection. The system maintains two copies of the root filesystem (Slot A and Slot B), allowing safe updates without risking the current working system.

## Architecture

### Partition Layout

The A/B system uses the following partition structure:

```
┌─────────────────────────────────────────────────┐
│ Partition 1: BOOT (FAT32)                      │
│  - rootfs-a.squashfs (Slot A)                  │
│  - rootfs-b.squashfs (Slot B)                  │
│  - Image (kernel)                               │
│  - initramfs                                    │
│  - Device trees                                 │
│  - extlinux configuration                       │
└─────────────────────────────────────────────────┘
┌─────────────────────────────────────────────────┐
│ Partition 2: Overlay (ext4)                    │
│  - Persistent user data                         │
│  - A/B metadata (active slot, boot counters)    │
│  - Configuration files                          │
└─────────────────────────────────────────────────┘
```

### Boot Flow

1. **Initramfs** reads A/B metadata from overlay partition
2. Checks for pending slot switch (for new OTA updates)
3. Increments boot counter for active slot
4. Checks if boot counter exceeds threshold (3 attempts)
5. Falls back to previous slot if needed
6. Mounts the appropriate squashfs image (rootfs-a or rootfs-b)
7. Switches to real root filesystem

### Boot Verification

After successful boot, the `ab-boot-verify.service` runs to mark the boot as successful, resetting the boot counter. If the system crashes or fails to reach multi-user.target 3 times, it automatically falls back to the previous slot.

## Building A/B Images

### Create A/B Image

Instead of the standard `make img`, use:

```bash
cd output.<board>
make imgab
```

This creates `tinilinux-<board>-ab.img` with both Slot A and Slot B pre-populated with identical root filesystems.

### Boot Parameters

To enable A/B boot logic, add `ab_boot=1` to the kernel command line. This is automatically included in QEMU targets:

```bash
make runqemuab      # Run A/B image in QEMU (console)
make runqemuguiab   # Run A/B image in QEMU (GUI)
```

## Using A/B OTA Updates

### Check Current Status

```bash
ab-slot-mgr.sh status
```

Output:
```
=== A/B Boot Slot Status ===

Active slot: a
Pending slot: none
Fallback enabled: 1

--- Slot A ---
Boot count: 0
Boot successful: 1

--- Slot B ---
Boot count: 0
Boot successful: 0
```

### Perform OTA Update

Download and install an update from a URL:

```bash
ota-update.sh update http://example.com/tinilinux-update.tar.gz [sha256sum]
```

The update package should be a tar.gz containing:
- `rootfs.squashfs` (required)
- `Image` (optional - kernel)
- `initramfs` (optional - initramfs)
- `dtb/` (optional - device trees)

The update will:
1. Download the package
2. Verify checksum (if provided)
3. Extract the package
4. Install to the inactive slot
5. Mark the inactive slot as pending for next boot

### Reboot to Apply Update

```bash
systemctl reboot
```

On next boot:
- System switches to the new slot
- Boot counter starts at 0
- If boot succeeds, counter resets
- If boot fails 3 times, system falls back to previous slot

### Manual Rollback

To rollback to the previous slot without waiting for automatic fallback:

```bash
ota-update.sh rollback
systemctl reboot
```

### Advanced Operations

**Get active slot:**
```bash
ab-slot-mgr.sh get-active
```

**Get inactive slot:**
```bash
ab-slot-mgr.sh get-inactive
```

**Mark current boot as successful (normally done automatically):**
```bash
ab-slot-mgr.sh mark-successful
```

**Set specific slot as pending:**
```bash
ab-slot-mgr.sh set-pending b
```

**Get detailed slot info:**
```bash
ab-slot-mgr.sh get-slot-info a
ab-slot-mgr.sh get-slot-info b
```

## Creating Update Packages

To create an OTA update package:

```bash
# Build your updated system
cd output.<board>
make

# Create update package
cd images
tar -czf update.tar.gz rootfs.squashfs Image initramfs

# Generate checksum
sha256sum update.tar.gz
```

## Safety Features

### Automatic Fallback

If the system fails to boot successfully 3 times in a row, it automatically falls back to the previous slot. This protects against:
- Corrupted updates
- Incompatible kernels
- Broken system services
- Any other boot failures

### Boot Verification

The system isn't marked as successfully booted until it reaches `multi-user.target` and the `ab-boot-verify.service` runs. This ensures the system is fully functional before resetting boot counters.

### Persistent Data Protection

User data in the overlay partition is never touched during updates. Only the read-only root filesystem is replaced.

## Migrating from Single Slot

To migrate an existing board configuration to A/B:

1. **Update extlinux.conf** to add `ab_boot=1` parameter
2. **Build with A/B image builder:**
   ```bash
   make imgab
   ```
3. **Flash the new A/B image**

The system will automatically initialize A/B metadata on first boot.

## Troubleshooting

### Check boot logs
```bash
journalctl -u ab-boot-verify.service
```

### View A/B metadata directly
```bash
cat /mnt/overlayfs/ab-metadata
```

### Force boot to specific slot
Edit `/mnt/overlayfs/ab-metadata` and change `ACTIVE_SLOT` and `PENDING_SLOT` as needed, then reboot.

### Disable automatic fallback
```bash
# Edit /mnt/overlayfs/ab-metadata
FALLBACK_ENABLED=0
```

## Example Workflow

```bash
# 1. Check current status
ab-slot-mgr.sh status
# Output: Active slot: a

# 2. Install update
ota-update.sh update http://myserver.com/update.tar.gz

# 3. Reboot
systemctl reboot

# 4. After reboot, check status
ab-slot-mgr.sh status
# Output: Active slot: b, Boot successful: 1

# 5. If there's a problem, rollback
ota-update.sh rollback
systemctl reboot
```

## Notes

- Both slots occupy space on the BOOT partition, so the partition needs to be sized accordingly (typically 2x original size)
- The inactive slot can be updated at any time without affecting the running system
- Updates are atomic - the old slot remains untouched until you reboot
- Network connectivity required for OTA updates
- Consider implementing signed updates for production use
