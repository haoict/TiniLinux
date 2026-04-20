#!/bin/bash

echo "  ✓ This script runs after creating filesystem images"

if [ -f "${BINARIES_DIR}/rootfs.squashfs" ]; then
    echo "    - Rename rootfs.squashfs to root.img"
    mv ${BINARIES_DIR}/rootfs.squashfs ${BINARIES_DIR}/root.img > /dev/null
fi