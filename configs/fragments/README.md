
# Config Fragments

TiniLinux uses a fragment-based configuration system to reduce duplication across board defconfigs.

## Fragment Structure

Fragments are located in `configs/fragments/`:
- **common.fragment** - Shared settings for all boards (toolchain, systemd, packages, etc.)
- **h700.fragment** / **rgb30.fragment** - Board-specific settings (kernel, U-Boot, firmware)
- **with-graphics.fragment** - GUI packages (SDL2, Mesa, ALSA, RetroArch, etc.)
- **squashfs.fragment** - Squashfs rootfs configuration
- **sway.fragment** - Sway compositor settings

## How It Works

Defconfigs reference fragments using `BR2_DEFCONFIG_FRAGMENT`:
```bash
BR2_DEFCONFIG_FRAGMENT="$(BR2_EXTERNAL_TiniLinux_PATH)/configs/fragments/common.fragment $(BR2_EXTERNAL_TiniLinux_PATH)/configs/fragments/h700.fragment $(BR2_EXTERNAL_TiniLinux_PATH)/configs/fragments/with-graphics.fragment"
BR2_ROOTFS_OVERLAY="$(BR2_EXTERNAL_TiniLinux_PATH)/board/common/rootfs $(BR2_EXTERNAL_TiniLinux_PATH)/board/h700/rootfs"
```

When you run `./make-board-build.sh configs/<board>_defconfig`, it automatically merges all referenced fragments.

## Saving Configuration Changes

After making changes via `make menuconfig`, use:
```bash
make savefragmentdefconfig
```

This preserves the fragment structure and only saves board-specific settings to the defconfig, not the content already in fragments.

**Note:** Do NOT use `make savedefconfig` for fragment-based configs as it will expand all fragments into the defconfig file.
