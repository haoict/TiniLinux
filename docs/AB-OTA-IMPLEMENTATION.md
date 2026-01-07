# A/B OTA Implementation Summary

## Overview

A complete A/B OTA (Over-The-Air) update system has been implemented for TiniLinux with automatic fallback protection, seamless updates, and boot verification.

## Files Created

### Core Scripts (`board/common/ota/`)

1. **ab-slot-mgr.sh** - A/B slot metadata manager (310 lines)
   - Manages active/inactive slot selection
   - Tracks boot counters and success status
   - Implements automatic fallback after 3 failed boots
   - Handles slot switching and pending updates

2. **ota-update.sh** - OTA update manager (215 lines)
   - Downloads OTA packages with checksum verification
   - Extracts and installs to inactive slot
   - Activates pending slots for next boot
   - Provides rollback functionality
   - Full workflow: `update`, `rollback`, `status` commands

3. **mk-ota-package.sh** - OTA package builder (200 lines)
   - Creates update packages from Buildroot output
   - Includes rootfs.squashfs, kernel, initramfs, DTBs
   - Generates manifest and SHA256 checksums
   - Usage: `./mk-ota-package.sh --board <board>`

4. **ab-boot-verify.service** - Systemd boot verification service
   - Runs after multi-user.target
   - Marks successful boots
   - Resets boot counters

5. **ota-update.service** - Systemd OTA update service template

6. **firstboot-ab.sh** - First boot initialization script

7. **extlinux.conf.ab-example** - Sample bootloader configurations

8. **README.md** - Documentation for OTA scripts

### Build Scripts

9. **mk-flashable-img-squashfs-ab.sh** - A/B image builder (150 lines)
   - Creates A/B partition layout with both slots
   - Doubles BOOT partition size for two squashfs images
   - Installs OTA scripts and services to overlay
   - Copies rootfs-a.squashfs and rootfs-b.squashfs

### Documentation

10. **docs/AB-OTA-SYSTEM.md** - Complete system documentation (400 lines)
    - Architecture overview
    - Boot flow explanation
    - Usage instructions
    - Safety features
    - Troubleshooting guide

11. **docs/AB-OTA-QUICKSTART.md** - Quick start guide (100 lines)
    - Common commands
    - Quick reference
    - Example workflows

## Files Modified

### 1. `package/initramfs/init`

**Changes:**
- Added A/B slot management functions
- Added `ab_boot` kernel parameter support
- Implements slot selection logic on boot
- Increments boot counters
- Handles automatic fallback after 3 failures
- Switches to pending slot if set
- Backward compatible (works without ab_boot=1)

**Key additions:**
```bash
# New functions:
- get_ab_active_slot()
- increment_boot_count()
- check_and_switch_slot()

# New logic:
- Read metadata from /mnt/overlayfs/ab-metadata
- Select rootfs-a.squashfs or rootfs-b.squashfs based on active slot
- Fallback to legacy single squashfs if A/B images not found
```

### 2. `external.mk`

**New targets added:**
- `imgab` - Build A/B squashfs image
- `runqemuab` - Run A/B image in QEMU (console)
- `runqemuguiab` - Run A/B image in QEMU with GUI

**Changes:**
```makefile
imgab:
    # Calls mk-flashable-img-squashfs-ab.sh

runqemuab:
    # Adds ab_boot=1 to kernel cmdline
    # Uses tinilinux-$(BOARD)-ab.img

runqemuguiab:
    # GUI version with ab_boot=1
```

## Architecture

### Partition Layout

```
┌─────────────────────────────────────────────┐
│ Partition 1: BOOT (FAT32, 2x size)         │
│  ├── rootfs-a.squashfs  (Slot A)           │
│  ├── rootfs-b.squashfs  (Slot B)           │
│  ├── Image (kernel)                         │
│  ├── initramfs                              │
│  └── extlinux/extlinux.conf                │
└─────────────────────────────────────────────┘
┌─────────────────────────────────────────────┐
│ Partition 2: Overlay (ext4)                │
│  ├── overlay_upper/ (user data)            │
│  ├── overlay_workdir/                      │
│  ├── ab-metadata (slot state)              │
│  └── ab-metadata.bak                       │
└─────────────────────────────────────────────┘
```

### Boot Flow

1. **U-Boot** loads kernel + initramfs, passes `ab_boot=1`
2. **Initramfs** mounts overlay temporarily
3. Reads `/mnt/overlayfs/ab-metadata`
4. Checks for pending slot switch
5. Checks if boot count exceeds 3 (fallback condition)
6. Increments boot counter for active slot
7. Selects `rootfs-a.squashfs` or `rootfs-b.squashfs`
8. Mounts squashfs + overlay
9. Switch to real root
10. **Systemd** boots system
11. **ab-boot-verify.service** marks boot successful
12. Boot counter resets to 0

### OTA Update Flow

```
┌─────────────────────────────────────────┐
│  1. Check Status (slot A active)       │
│     $ ab-slot-mgr.sh status             │
└──────────────┬──────────────────────────┘
               │
┌──────────────▼──────────────────────────┐
│  2. Download & Install to Slot B       │
│     $ ota-update.sh update URL          │
│     - Downloads update.tar.gz           │
│     - Extracts to /tmp                  │
│     - Copies to /boot/rootfs-b.squashfs │
│     - Marks slot B as pending           │
└──────────────┬──────────────────────────┘
               │
┌──────────────▼──────────────────────────┐
│  3. Reboot                              │
│     $ systemctl reboot                  │
└──────────────┬──────────────────────────┘
               │
┌──────────────▼──────────────────────────┐
│  4. Initramfs switches to Slot B        │
│     - Reads pending slot from metadata  │
│     - Activates slot B                  │
│     - Mounts rootfs-b.squashfs          │
└──────────────┬──────────────────────────┘
               │
┌──────────────▼──────────────────────────┐
│  5. System boots into Slot B            │
│     - Boot counter = 1                  │
│     - If successful, marked at multi-   │
│       user.target                       │
└──────────────┬──────────────────────────┘
               │
     ┌─────────▼──────────┐
     │ Success?           │
     └─────────┬──────────┘
          Yes  │  No (3 times)
               │
      ┌────────▼─────────┐
      │ Mark Successful  │   ┌──────────────────┐
      │ Reset counter    │   │ Auto Fallback    │
      └──────────────────┘   │ to Slot A        │
                              └──────────────────┘
```

## Safety Features

1. **Automatic Fallback**
   - System tracks boot attempts
   - After 3 failed boots, automatically reverts to previous slot
   - Protects against broken updates

2. **Boot Verification**
   - Update not permanent until system reaches multi-user.target
   - `ab-boot-verify.service` must run to mark boot successful
   - Ensures full system functionality before committing

3. **Atomic Updates**
   - Updates install to inactive slot
   - Current system completely untouched until reboot
   - Can cancel update before rebooting

4. **Data Preservation**
   - Overlay partition never modified during updates
   - User data, configs, and state preserved
   - Only read-only root filesystem updated

## Usage

### Building A/B Images

```bash
cd output.pc_qemu_aarch64_virt
make imgab
make runqemuab
```

### Creating Updates

```bash
# After building new version
./board/common/ota/mk-ota-package.sh --board pc_qemu_aarch64_virt

# Outputs:
# ota-packages/tinilinux-pc_qemu_aarch64_virt-20260107-123456.tar.gz
# ota-packages/tinilinux-pc_qemu_aarch64_virt-20260107-123456.sha256
```

### Installing Updates

```bash
# On device
ota-update.sh update http://myserver.com/update.tar.gz abc123...
systemctl reboot
```

### Checking Status

```bash
ab-slot-mgr.sh status
```

### Rolling Back

```bash
ota-update.sh rollback
systemctl reboot
```

## Enabling A/B Boot

Add to extlinux.conf:

```
APPEND initrd=/initramfs bootpart=/dev/mmcblk0p1 squashfsimg=rootfs.squashfs overlayfs=/dev/mmcblk0p2 ab_boot=1 rootwait rw
```

## Backward Compatibility

- System works with existing single-slot images (without `ab_boot=1`)
- If A/B images not found, falls back to legacy `rootfs.squashfs`
- Existing boards continue to work without changes
- A/B is opt-in via kernel parameter

## Testing

Tested with QEMU virt board:

```bash
cd output.pc_qemu_aarch64_virt
make imgab
make runqemuab

# In QEMU:
ab-slot-mgr.sh status
# Test slot switching, fallback, etc.
```

## Future Enhancements

Potential improvements:
- [ ] Signed OTA packages with signature verification
- [ ] Delta updates (binary diffs between slots)
- [ ] Update server with automatic discovery
- [ ] Update scheduling and staged rollouts
- [ ] Kernel A/B support (separate Image-a, Image-b)
- [ ] Bootloader updates (U-Boot A/B)
- [ ] Network failover during download
- [ ] Update progress reporting
- [ ] Integration with monitoring systems

## Integration Checklist

To add A/B support to a new board:

- [ ] Ensure squashfs-based rootfs
- [ ] Verify BOOT partition size is adequate (needs 2x space)
- [ ] Add `ab_boot=1` to extlinux.conf
- [ ] Build with `make imgab`
- [ ] Test slot switching
- [ ] Test automatic fallback
- [ ] Test OTA updates
- [ ] Document board-specific details

## Summary

This implementation provides a production-ready A/B OTA update system with:
- ✅ Automatic fallback protection
- ✅ Boot verification
- ✅ Seamless updates
- ✅ Data preservation
- ✅ Complete tooling (build, deploy, manage)
- ✅ Comprehensive documentation
- ✅ QEMU testing support
- ✅ Backward compatibility

All components are integrated into the existing Buildroot-based TiniLinux build system and follow the project's conventions.
