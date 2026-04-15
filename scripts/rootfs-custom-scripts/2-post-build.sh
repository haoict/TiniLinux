#!/bin/bash

echo "  ✓ This script runs before creating filesystem images"

echo "    - Updating build info"
sed -i "s/^BUILD_ID=.*/BUILD_ID=$(TZ='Asia/Tokyo' date +%Y%m%d-%H%M)JST/" ${TARGET_DIR}/etc/os-release
cat ${TARGET_DIR}/etc/os-release
echo ""
