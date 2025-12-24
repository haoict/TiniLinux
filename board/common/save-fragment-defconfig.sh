#!/bin/bash

set -euo pipefail

# This script saves a defconfig while preserving BR2_DEFCONFIG_FRAGMENT
# Usage: run from output.<board> directory

# Get the TiniLinux root directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"

# Check if we're in an output directory
if [[ ! "$PWD" =~ output\. ]]; then
    echo "Error: This script must be run from an output.<board> directory"
    echo "Example: cd output.h700 && ../save-fragment-defconfig.sh"
    exit 1
fi

# Check if .config exists
if [ ! -f .config ]; then
    echo "Error: .config not found in current directory"
    exit 1
fi

# Get the defconfig path from BR2_DEFCONFIG
DEFCONFIG_PATH=$(grep "^BR2_DEFCONFIG=" .config | sed 's/BR2_DEFCONFIG=//' | tr -d '"')

if [ -z "$DEFCONFIG_PATH" ]; then
    echo "Error: BR2_DEFCONFIG not found in .config"
    exit 1
fi

echo "Target defconfig: $DEFCONFIG_PATH"

# Check if the original defconfig uses fragments
if [ ! -f "$DEFCONFIG_PATH" ]; then
    echo "Error: Defconfig not found: $DEFCONFIG_PATH"
    exit 1
fi

if ! grep -q "BR2_DEFCONFIG_FRAGMENT=" "$DEFCONFIG_PATH"; then
    echo "This defconfig doesn't use fragments, using regular savedefconfig"
    make savedefconfig
    exit 0
fi

echo "Defconfig uses fragments, preserving fragment structure..."

# Extract fragment line and rootfs overlay from original defconfig
FRAGMENT_LINE=$(grep "BR2_DEFCONFIG_FRAGMENT=" "$DEFCONFIG_PATH")
OVERLAY_LINE=$(grep "BR2_ROOTFS_OVERLAY=" "$DEFCONFIG_PATH" || true)

# Save current config to a temp file
TEMP_SAVED=$(mktemp)
make BR2_DEFCONFIG="$TEMP_SAVED" savedefconfig

echo "Saved minimal config to temp file"

# Create temp file to merge all fragments
TEMP_FRAGMENTS=$(mktemp)

# Extract and expand fragment paths
FRAGMENTS=$(echo "$FRAGMENT_LINE" | sed 's/BR2_DEFCONFIG_FRAGMENT=//' | tr -d '"')
FRAGMENTS=$(echo "$FRAGMENTS" | sed "s|\$(BR2_EXTERNAL_TiniLinux_PATH)|$SCRIPT_DIR|g")

# Merge all fragments
for fragment in $FRAGMENTS; do
    if [ -f "$fragment" ]; then
        cat "$fragment" >> "$TEMP_FRAGMENTS"
        echo "" >> "$TEMP_FRAGMENTS"
    fi
done

echo "Merged all fragments"

# Find settings that are in saved config but not in fragments
# This will be our defconfig-specific settings
TEMP_UNIQUE=$(mktemp)

while IFS= read -r line; do
    # Skip empty lines and comments
    if [[ -z "$line" || "$line" =~ ^# ]]; then
        continue
    fi
    
    # Extract the config key (e.g., BR2_PACKAGE_HTOP from BR2_PACKAGE_HTOP=y)
    if [[ "$line" =~ ^(BR2_[A-Z0-9_]+) ]]; then
        CONFIG_KEY="${BASH_REMATCH[1]}"
        
        # Check if this key is in fragments
        if ! grep -q "^$CONFIG_KEY" "$TEMP_FRAGMENTS"; then
            # This is unique to this defconfig
            echo "$line" >> "$TEMP_UNIQUE"
        fi
    fi
done < "$TEMP_SAVED"

echo "Identified unique settings"

# Create the new defconfig
{
    echo "$FRAGMENT_LINE"
    echo "$OVERLAY_LINE"
    if [ -s "$TEMP_UNIQUE" ]; then
        cat "$TEMP_UNIQUE"
    fi
} > "$DEFCONFIG_PATH"

# Clean up
rm -f "$TEMP_SAVED" "$TEMP_FRAGMENTS" "$TEMP_UNIQUE"

echo "Successfully saved defconfig with fragments preserved: $DEFCONFIG_PATH"
