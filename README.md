[![Build](https://github.com/haoict/TiniLinux/actions/workflows/build.yaml/badge.svg?branch=master)](https://github.com/haoict/TiniLinux/actions/workflows/build.yaml)

# Tinilinux
"Tini" Linux distro for H700, RK3326 & RK3566 SOC devices

# Boards & defconfig

| Board Name                      | CPU/Arch             | GPU      | Kernel  | Init    | Notes                                                                              |
| ------------------------------- | -------------------- | -------- | ------- | ------- | ---------------------------------------------------------------------------------- |
| rgb30                           | aarch64 (Cortex-A55) | Panfrost | 6.12.43 | systemd | Rockchip, EGL/ES, U-Boot, SDL2 KSMDRM, Python3, OpenSSL, SSH                       |
| h700                            | aarch64 (Cortex-A53) | Panfrost | 6.16.9  | systemd | Sun50i, EGL/ES, U-Boot, SDL2 KSMDRM, Python3, OpenSSL, SSH                         |
| h700_sway                       | aarch64 (Cortex-A53) | Panfrost | 6.16.9  | systemd | same as h700 but with sway compositor instead of KMSDRM                            |
| xxx_consoleonly                 | aarch64              | N/A      | -       | systemd | include only base components for console, no GPU and GUI apps                      |
| xxx_squashfs                    | aarch64              | Panfrost | -       | systemd | same as rgb30, h700,... (with GUI) but uses squashfs for rootfs                    |
| toolchain_$hostarch_$targetarch | N/A                  | N/A      | N/A     | N/A     | install toolchain only to build separated packages purpose, not a full board build |

# Build
Clone TiniLinux and buildroot repo and setup environments
```bash
# Clone sources
git clone https://github.com/haoict/TiniLinux.git
git clone --depth=1 -b 2025.11 https://github.com/buildroot/buildroot.git

# Install required packages
sudo apt install build-essential libncurses-dev dosfstools parted mtools

# Create board config
cd buildroot
make O=../TiniLinux/output.<boardname> BR2_EXTERNAL=../TiniLinux <boardname>_defconfig

# Build
cd ../TiniLinux/output.<boardname>
make menuconfig # adjust anything if you want, otherwise just exit
make -j$(nproc)
# The kernel, bootloader, root filesystem, etc. are in output images directory
# Make flashable img file
make img
# or: sudo BOARD=<boardname> board/common/mk-flashable-img.sh
```

# Install
## Flash to sdcard
There are many tools to flash img file to SDCard such as Rufus, Balena Etcher.
But if you prefer command line:
```bash
# Check your sdcard device
lsblk
# Assuming your sdcard is /dev/sdb, unmount all mounted partitions of your sdcard if automount 
sudo umount /dev/sdb*
# Flash the img to sdcard
sudo dd if=output.${BOARD}/images/tinilinux-${BOARD}.img of=/dev/sdb bs=4M conv=fsync status=progress
sudo sync
sudo udisksctl power-off -b /dev/sdb
```

## Update rootfs only without reflashing sdcard
```bash
sudo mount -t ext4 /dev/sdb /mnt/rootfs
sudo rm -rf /mnt/rootfs/*
sudo tar -xvf output.${BOARD}/images/rootfs.tar -C /mnt/rootfs && sync
sudo umount /dev/sdb
sudo eject /dev/sdb
```

# Notes

## Build from docker container

If it's inconvernient to build directly in host machine, you can build TiniLinux inside a docker container
```bash
# Clone sources
git clone https://github.com/haoict/TiniLinux.git
git clone --depth=1 -b 2025.11 https://github.com/buildroot/buildroot.git

# First build the image
cd TiniLinux
docker build -t ghcr.io/haoict/tinilinux-builder:latest .
cd ..
docker run --name tinilinux-builder -d -v $(pwd):/home/ubuntu ghcr.io/haoict/tinilinux-builder:latest
docker exec -it tinilinux-builder bash

# NOTE: Commands from here are executed inside docker container
# Create board config
cd buildroot
make O=../TiniLinux/output.<boardname> BR2_EXTERNAL=../TiniLinux <boardname>_defconfig

# Build
cd ../TiniLinux/output.<boardname>
make -j$(nproc)
make img
```

## Clean target build without rebuild all binaries and libraries
Ref: https://stackoverflow.com/questions/47320800/how-to-clean-only-target-in-buildroot

```bash
# list all built packages
cd output.${BOARD}
make show-targets

# clean some packages that usually change
make alsa-lib-dirclean alsa-plugins-dirclean alsa-utils-dirclean btop-dirclean dingux-commander-dirclean gptokeyb2-dirclean retroarch-dirclean rocknix-joypad-dirclean sdl12-compat-dirclean sdl2-dirclean simple-launcher-dirclean simple-terminal-dirclean systemd-dirclean tinilinux-initramfs-dirclean wayland-dirclean wayland-protocols-dirclean wpa_supplicant-dirclean

# clean target without rebuild: make clean-target
rm -rf target && find  -name ".stamp_target_installed" -delete && rm -f build/host-gcc-final-*/.stamp_host_installed
```

## Unpack/Repack initramfs
Unpack
```bash
mkdir initramfs-files
cd initramfs-files
zcat ../initramfs | cpio -id
```

Repack
```bash
find . | cpio -o -H newc | gzip > ../initramfs-modified.cpio.gz
```

## Run Docker
```bash
# All versions can be found here: https://download.docker.com/linux/static/stable/aarch64/
wget https://download.docker.com/linux/static/stable/aarch64/docker-29.0.1.tgz
tar -xzvf docker-29.0.1.tgz
mv docker/* /usr/bin/
dockerd &
docker run -p 8080:80 -d --name hello --rm nginxdemos/hello
docker ps -a
curl localhost:8080
```