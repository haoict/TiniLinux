#!/bin/bash

echo "  ✓ This script runs before creating filesystem images"

echo "  ✓ Fix users permissions"
chmod +s ${TARGET_DIR}/usr/bin/sudo
chmod +s ${TARGET_DIR}/usr/bin/passwd
ls -la ${TARGET_DIR}/usr/bin/sudo
ls -la ${TARGET_DIR}/usr/bin/passwd
# chown -R admin:tinilinux ${TARGET_DIR}/home/admin || true
# chown -R viewer:tinilinux ${TARGET_DIR}/home/viewer || true

echo "  ✓ Updating build info"
sed -i "s/^BUILD_ID=.*/BUILD_ID=$(TZ='Asia/Tokyo' date +%Y%m%d-%H%M)JST/" ${TARGET_DIR}/etc/os-release
cat ${TARGET_DIR}/etc/os-release
