#!/bin/bash

echo "  ✓ This script runs before creating filesystem images"


echo "    - Remove docker.service on boot"
rm -f ${TARGET_DIR}/etc/systemd/system/multi-user.target.wants/docker.service
echo ""

echo "    - Remove mail check"
sed -i 's/^MAIL_CHECK_ENAB.*/MAIL_CHECK_ENAB\t\tno/' ${TARGET_DIR}/etc/login.defs
echo ""

echo "    - Copy initramfs.img to /usr/local/lib/ for use in initramfs-shutdown script"
cp ${BINARIES_DIR}/initramfs ${TARGET_DIR}/usr/local/lib/initramfs.img
echo ""
