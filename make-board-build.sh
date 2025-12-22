#!/bin/bash

set -euo pipefail

# Check if defconfig path is provided
if [ $# -ne 1 ]; then
    echo "Usage: $0 <defconfig_path>"
    echo "Example: $0 configs/h700_squashfs_defconfig"
    exit 1
fi

DEFCONFIG_PATH="$1"

# check if defconfig file exists
if [ ! -f "$DEFCONFIG_PATH" ]; then
    echo "Error: Defconfig file not found: $DEFCONFIG_PATH"
    exit 1
fi

# Extract boardname from defconfig path
# e.g., configs/h700_squashfs_defconfig -> h700_squashfs
BOARDNAME=$(basename "$DEFCONFIG_PATH" _defconfig)

echo "Board name: $BOARDNAME"

# Check if ../buildroot exists, if not clone it
if [ ! -d "../buildroot" ]; then
    echo "Cloning buildroot repository..."
    git clone --depth=1 -b 2025.11 https://github.com/buildroot/buildroot.git ../buildroot
fi

# Get the directory of this script (TiniLinux)
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Check if defconfig uses BR2_DEFCONFIG_FRAGMENT
if grep -q "BR2_DEFCONFIG_FRAGMENT" "$SCRIPT_DIR/$DEFCONFIG_PATH"; then
    echo "Defconfig uses fragments, merging..."
    
    # Create a temporary merged defconfig
    TEMP_DEFCONFIG=$(mktemp)
    
    # Extract fragment files from BR2_DEFCONFIG_FRAGMENT line
    FRAGMENTS=$(grep "BR2_DEFCONFIG_FRAGMENT=" "$SCRIPT_DIR/$DEFCONFIG_PATH" | sed 's/BR2_DEFCONFIG_FRAGMENT=//' | tr -d '"')
    
    # Expand $(BR2_EXTERNAL_TiniLinux_PATH) to actual path
    FRAGMENTS=$(echo "$FRAGMENTS" | sed "s|\$(BR2_EXTERNAL_TiniLinux_PATH)|$SCRIPT_DIR|g")
    
    # Merge all fragments
    for fragment in $FRAGMENTS; do
        if [ -f "$fragment" ]; then
            echo "  Including fragment: $(basename $fragment)"
            cat "$fragment" >> "$TEMP_DEFCONFIG"
            echo "" >> "$TEMP_DEFCONFIG"
        else
            echo "  Warning: Fragment not found: $fragment"
        fi
    done
    
    # Append non-fragment lines from the original defconfig
    grep -v "BR2_DEFCONFIG_FRAGMENT=" "$SCRIPT_DIR/$DEFCONFIG_PATH" >> "$TEMP_DEFCONFIG"
    
    # Add BR2_DEFCONFIG to point to the correct defconfig with absolute path
    echo "BR2_DEFCONFIG=\"$SCRIPT_DIR/configs/${BOARDNAME}_defconfig\"" >> "$TEMP_DEFCONFIG"
    
    # Create output directory and copy merged config
    mkdir -p "../TiniLinux/output.${BOARDNAME}"
    cp "$TEMP_DEFCONFIG" "../TiniLinux/output.${BOARDNAME}/.config"
    
    # Go to buildroot and execute make with the merged config
    echo "Creating board configuration..."
    cd ../buildroot
    make O=../TiniLinux/output.${BOARDNAME} BR2_EXTERNAL=../TiniLinux olddefconfig
    
    # Clean up
    rm -f "$TEMP_DEFCONFIG"
else
    # Traditional defconfig without fragments
    echo "Creating board configuration..."
    cd ../buildroot
    make O=../TiniLinux/output.${BOARDNAME} BR2_EXTERNAL=../TiniLinux ${BOARDNAME}_defconfig
fi

echo "Board configuration created successfully!"
echo "Next steps:"
echo "  cd output.${BOARDNAME}"
echo "  make -j\$(nproc)"

