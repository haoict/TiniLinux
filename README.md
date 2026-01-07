[![Build](https://github.com/haoict/TiniLinux/actions/workflows/build.yaml/badge.svg?branch=master)](https://github.com/haoict/TiniLinux/actions/workflows/build.yaml)

# Tinilinux

"Tini" Linux distro for H700, RK3326 & RK3566 SOC devices

# Boards & defconfig

| Board Name                    | CPU/Arch             | GPU      | Kernel | Init    | Rootfs           | Notes                                                                                   |
| ----------------------------- | -------------------- | -------- | ------ | ------- | ---------------- | --------------------------------------------------------------------------------------- |
| rgb30                         | aarch64 (Cortex-A55) | Panfrost | 6.18.2 | systemd | squashfs/overlay | Rockchip, EGL/ES, U-Boot, SDL2 KSMDRM, Python3, OpenSSL, SSH, Retroarch                 |
| h700                          | aarch64 (Cortex-A53) | Panfrost | 6.18.2 | systemd | squashfs/overlay | Sun50i, EGL/ES, U-Boot, SDL2 KSMDRM, Python3, OpenSSL, SSH, Retroarch                   |
| xxx_rootrw                    | -                    | Panfrost | -      | systemd | ext4 (rw)        | uses ext4 read-write rootfs instead of squashfs                                         |
| xxx_consoleonly               | -                    | N/A      | -      | systemd | squashfs/overlay | include only base components for console, no GPU and GUI apps                           |
| xxx_sway                      | -                    | Panfrost | -      | systemd | squashfs/overlay | uses sway compositor instead of KMSDRM, helps to deal with RG28xx screen rotation issue |
| pc_qemu_aarch64_virt          | aarch64              | virgl    | -      | systemd | squashfs/overlay | build kernel, initramfs, rootfs to test wit qemu                                        |
| toolchain_hostArch_targetArch | N/A                  | N/A      | N/A    | N/A     | N/A              | install toolchain only to build separated packages purpose, not a full board build      |

# Build

Clone TiniLinux and buildroot repo and setup environments

```bash
# Install required packages
sudo apt update
sudo apt install build-essential cmake mtools libncurses-dev dosfstools parted

# Clone sources
git clone https://github.com/haoict/TiniLinux.git

# Create board config
cd TiniLinux
./make-board-build.sh configs/<boardname>_defconfig

# Build
cd output.<boardname>
make menuconfig # adjust anything if you want, otherwise just exit. If you add/remove packages, you can save changes with "make savefconf" command to update board's defconfig file.
make -j$(nproc)
## The kernel, bootloader, root filesystem, etc. are in output images directory

# Make flashable img file
make img
```

# Install

## Flash to sdcard

There are many tools to flash img file to SDCard such as Rufus, Balena Etcher.
But if you prefer command line:

```bash
make flash
```

## Update rootfs only without reflashing sdcard

```bash
sudo mount -t ext4 /dev/sdb /mnt/rootfs
sudo rm -rf /mnt/rootfs/*
sudo tar -xvf images/rootfs.tar -C /mnt/rootfs && sync
sudo umount /dev/sdb
sudo eject /dev/sdb
```

# Notes

## Build from docker container

If it's inconvernient to build directly in host machine, for example MacOS host, you can build TiniLinux inside a docker container

```bash
# Clone sources
git clone https://github.com/haoict/TiniLinux.git

cd TiniLinux

# Build the image and run container (if the image already built and the container already ran, skip to the "docker exec..." command below)
docker build -t ghcr.io/haoict/tinilinux-builder:latest .
docker run --name tinilinux-builder -d -v $(pwd):/home/ubuntu/TiniLinux -v tinilinux-builder-buildroot:/home/ubuntu/buildroot ghcr.io/haoict/tinilinux-builder:latest

docker exec -it tinilinux-builder bash

# NOTE: Commands from here are executed inside docker container
cd TiniLinux
./make-board-build.sh configs/<boardname>_defconfig docker
cd /home/ubuntu/buildroot/output.<boardname>
make -j$(nproc)
make img

# copy output images dir to the TiniLinux folder
cd /home/ubuntu/TiniLinux
mkdir -p output.<boardname>
cp -r /home/ubuntu/buildroot/output.<boardname>/images output.<boardname>
```

## Test with qemu

With pc_qemu_targetArch_virt build, we can test kernel, initramfs, rootfs disk with qemu

```bash
sudo apt install qemu-system-aarch64
cd output.pc_qemu_aarch64_virt (or _consoleonly variant)
make -j$(nproc)
make img
make runqemu (or make runqemugui)
```

## Clean target build without rebuild all binaries and libraries

Ref: https://stackoverflow.com/questions/47320800/how-to-clean-only-target-in-buildroot

```bash
# list all built packages
cd output.${BOARD}
make show-targets

# clean some packages that usually change
make alsa-lib-dirclean alsa-plugins-dirclean alsa-utils-dirclean btop-dirclean dingux-commander-dirclean gptokeyb2-dirclean retroarch-dirclean rocknix-joypad-dirclean sdl2-dirclean simple-launcher-dirclean simple-terminal-dirclean systemd-dirclean tinilinux-initramfs-dirclean wayland-dirclean wayland-protocols-dirclean wpa_supplicant-dirclean

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
wget https://download.docker.com/linux/static/stable/aarch64/docker-29.1.3.tgz
tar -xzvf docker-29.1.3.tgz
mv docker/* /usr/bin/
dockerd &
docker run -p 8080:80 -d --name hello --rm nginxdemos/hello
docker ps -a
curl localhost:8080
```
