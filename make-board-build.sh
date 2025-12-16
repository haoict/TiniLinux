#!/bin/bash

set -euo pipefail

# Check if defconfig path is provided
if [ $# -ne 1 ]; then
    echo "Usage: $0 <defconfig_path>"
    echo "Example: $0 configs/h700_squashfs_defconfig"
    exit 1
fi

DEFCONFIG_PATH="$1"

# Extract boardname from defconfig path
# e.g., configs/h700_squashfs_defconfig -> h700_squashfs
BOARDNAME=$(basename "$DEFCONFIG_PATH" _defconfig)

echo "Board name: $BOARDNAME"

# Check if ../buildroot exists, if not clone it
if [ ! -d "../buildroot" ]; then
    echo "Cloning buildroot repository..."
    git clone --depth=1 -b 2025.11 https://github.com/buildroot/buildroot.git ../buildroot
fi

# Go to buildroot and execute make command
echo "Creating board configuration..."
cd ../buildroot
make O=../TiniLinux/output.${BOARDNAME} BR2_EXTERNAL=../TiniLinux ${BOARDNAME}_defconfig

echo "Board configuration created successfully!"
echo "Next steps:"
echo "  cd output.${BOARDNAME}"
echo "  make -j\$(nproc)"

