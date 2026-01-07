# TiniLinux A/B OTA Scripts

This directory contains the A/B OTA update system for TiniLinux.

## Files

### Core Scripts (deployed to devices)

- **ab-slot-mgr.sh** - A/B slot metadata manager
  - Manages active/inactive slots
  - Tracks boot counts and success status
  - Handles automatic fallback logic
  - Commands: `init`, `get-active`, `get-inactive`, `mark-successful`, `set-pending`, `status`, etc.

- **ota-update.sh** - OTA update manager
  - Downloads update packages
  - Installs updates to inactive slot
  - Activates pending slots
  - Commands: `download`, `extract`, `install`, `activate`, `update`, `rollback`, `status`

### Build-time Scripts

- **mk-ota-package.sh** - OTA package builder
  - Creates OTA update packages from Buildroot output
  - Includes rootfs.squashfs, kernel, initramfs, device trees
  - Generates manifest and checksums
  - Usage: `./mk-ota-package.sh --board <board>`

### Systemd Services

- **ab-boot-verify.service** - Boot verification service
  - Runs after successful boot to multi-user.target
  - Marks current boot as successful
  - Resets boot counters

- **ota-update.service** - OTA update service template
  - Framework for automated OTA updates
  - Not started automatically (manual trigger)

### Configuration Examples

- **extlinux.conf.ab-example** - Sample bootloader configs
  - Shows how to enable A/B boot with `ab_boot=1`
  - Examples for different boards (QEMU, H700, RGB30)

- **firstboot-ab.sh** - First boot initialization
  - Sets up A/B OTA infrastructure on first boot
  - Copies scripts, enables services, initializes metadata

## Integration

These scripts are integrated into the build system through:

1. **mk-flashable-img-squashfs-ab.sh** - Creates A/B partition layout
   - Copies both rootfs-a.squashfs and rootfs-b.squashfs to BOOT
   - Installs OTA scripts to overlay partition
   - Enables systemd services

2. **package/initramfs/init** - Modified to support A/B boot
   - Reads A/B metadata
   - Selects active slot
   - Handles pending slot switches
   - Implements fallback logic

3. **external.mk** - Build system integration
   - `make imgab` - Build A/B image
   - `make runqemuab` - Run A/B image in QEMU
   - `make runqemuguiab` - Run A/B image in QEMU with GUI

## Deployment

For a board to support A/B OTA:

1. Build with `make imgab` instead of `make img`
2. Flash the resulting A/B image
3. Add `ab_boot=1` to kernel cmdline in extlinux.conf
4. Scripts are automatically deployed to:
   - `/usr/local/bin/ab-slot-mgr.sh`
   - `/usr/local/bin/ota-update.sh`
   - `/etc/systemd/system/ab-boot-verify.service`

## Metadata Format

A/B metadata is stored in `/mnt/overlayfs/ab-metadata`:

```bash
# TiniLinux A/B Boot Slot Metadata
ACTIVE_SLOT=a              # Current active slot (a or b)
BOOT_COUNT_A=0             # Number of boot attempts for slot A
BOOT_COUNT_B=0             # Number of boot attempts for slot B
BOOT_SUCCESS_A=1           # 1 if slot A has booted successfully
BOOT_SUCCESS_B=0           # 1 if slot B has booted successfully
FALLBACK_ENABLED=1         # 1 to enable automatic fallback
PENDING_SLOT=              # Slot to switch to on next boot (empty = no switch)
```

## Update Package Format

OTA update packages are tar.gz archives containing:

```
update.tar.gz/
├── rootfs.squashfs    (required)
├── Image              (optional - kernel)
├── initramfs          (optional)
├── dtb/               (optional - device trees)
├── manifest.txt       (generated)
└── checksum.sha256    (generated)
```

## Testing

Test A/B OTA with QEMU:

```bash
# Build A/B image
cd output.pc_qemu_aarch64_virt
make imgab

# Run in QEMU
make runqemuab

# In QEMU, test OTA workflow:
ab-slot-mgr.sh status
# Create a fake update and test installation
```

## Documentation

- [AB-OTA-SYSTEM.md](../../docs/AB-OTA-SYSTEM.md) - Complete system documentation
- [AB-OTA-QUICKSTART.md](../../docs/AB-OTA-QUICKSTART.md) - Quick start guide

## License

Same as TiniLinux project - see LICENSE in repository root.
