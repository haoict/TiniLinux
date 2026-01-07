# A/B OTA Quick Start Guide

## What is A/B OTA?

A/B OTA (Over-The-Air) updates allow you to safely update TiniLinux without risking your current system. The system maintains two complete root filesystems (Slot A and Slot B) and automatically falls back if an update fails.

## Quick Commands

### Check Status
```bash
ab-slot-mgr.sh status
```

### Install Update
```bash
ota-update.sh update http://example.com/update.tar.gz [checksum]
```

### Reboot to Apply
```bash
systemctl reboot
```

### Rollback
```bash
ota-update.sh rollback
systemctl reboot
```

## Building A/B Images

```bash
cd output.<board>
make imgab              # Create A/B image
make runqemuab          # Test in QEMU (console)
make runqemuguiab       # Test in QEMU (GUI)
```

## Creating Update Packages

```bash
# After building your system
cd /home/haoict/TiniLinux
./board/common/ota/mk-ota-package.sh --board <board>

# Output will be in: ota-packages/tinilinux-<board>-<timestamp>.tar.gz
```

## Safety Features

✅ **Automatic Fallback**: System reverts to previous slot after 3 failed boot attempts  
✅ **Boot Verification**: Update isn't permanent until system boots successfully  
✅ **Atomic Updates**: Old system remains untouched until you reboot  
✅ **Data Preservation**: User data on overlay partition is never affected  

## Workflow Example

```bash
# 1. Check current slot
ab-slot-mgr.sh status
# Active slot: a

# 2. Download and install update to inactive slot (b)
ota-update.sh update http://myserver.com/update.tar.gz

# 3. Reboot to switch to new slot
systemctl reboot

# 4. System boots into slot b
# If successful after 3 reboots, slot b becomes permanent

# 5. If there's a problem, rollback
ota-update.sh rollback
systemctl reboot
```

## Enabling A/B Boot

Add `ab_boot=1` to your kernel command line in extlinux.conf:

```
APPEND initrd=/initramfs bootpart=/dev/mmcblk0p1 squashfsimg=rootfs.squashfs overlayfs=/dev/mmcblk0p2 ab_boot=1 rootwait rw
```

## Files and Scripts

- **ab-slot-mgr.sh** - Manages A/B slot metadata and switching
- **ota-update.sh** - Downloads and installs OTA updates
- **mk-ota-package.sh** - Creates OTA update packages from builds
- **ab-boot-verify.service** - Systemd service that marks boots as successful

## More Information

See [AB-OTA-SYSTEM.md](AB-OTA-SYSTEM.md) for complete documentation.
