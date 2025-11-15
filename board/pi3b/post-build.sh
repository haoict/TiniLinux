#!/bin/sh

set -u
set -e

# Add a console on tty1
if [ -e ${TARGET_DIR}/etc/inittab ]; then
    grep -qE '^tty1::' ${TARGET_DIR}/etc/inittab || \
	sed -i '/GENERIC_SERIAL/a\
tty1::respawn:/sbin/getty -L  tty1 0 vt100 # HDMI console' ${TARGET_DIR}/etc/inittab
# systemd doesn't use /etc/inittab, enable getty.tty1.service instead
elif [ -d ${TARGET_DIR}/etc/systemd ]; then
    mkdir -p "${TARGET_DIR}/etc/systemd/system/getty.target.wants"
    ln -sf /lib/systemd/system/getty@.service \
       "${TARGET_DIR}/etc/systemd/system/getty.target.wants/getty@tty1.service"
fi


sed -i "s/^BUILD_ID=buildroot/BUILD_ID=$(TZ='Asia/Tokyo' date +%Y%m%d-%H%M)JST/" ${TARGET_DIR}/etc/os-release
mktempdir=$(mktemp -d)
cp -r $BR2_EXTERNAL_TiniLinux_PATH/board/common/ROMS/ ${mktempdir}/
if [ -d $BR2_EXTERNAL_TiniLinux_PATH/board/pi3b/ROMS/ ]; then cp -r $BR2_EXTERNAL_TiniLinux_PATH/board/pi3b/ROMS/ ${mktempdir}/; fi
tar -Jcf ${TARGET_DIR}/root/roms.tar.xz -C ${mktempdir}/ROMS/ .
rm -rf ${mktempdir}
