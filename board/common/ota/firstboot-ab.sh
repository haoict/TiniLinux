#!/bin/sh
#
# firstboot.sh - A/B OTA support initialization
# This script runs on first boot to set up A/B OTA infrastructure
#

set -e

echo "Setting up A/B OTA support..."

# Copy OTA scripts to system
if [ ! -f /usr/local/bin/ab-slot-mgr.sh ]; then
    echo "Installing A/B slot manager..."
    # Scripts will be in overlay or copied during build
fi

# Enable boot verification service
if [ -f /etc/systemd/system/ab-boot-verify.service ]; then
    echo "Enabling A/B boot verification service..."
    systemctl enable ab-boot-verify.service
fi

# Initialize A/B metadata if not present
if [ ! -f /mnt/overlayfs/ab-metadata ]; then
    echo "Initializing A/B metadata..."
    /usr/local/bin/ab-slot-mgr.sh init
fi

echo "A/B OTA support initialized"
