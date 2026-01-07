#!/bin/bash
#
# A/B OTA Demo Script
# Demonstrates the A/B OTA update workflow
#

set -e

echo "================================================"
echo "  TiniLinux A/B OTA Update System Demo"
echo "================================================"
echo ""

# Check if we're running with A/B support
if [ ! -f /usr/local/bin/ab-slot-mgr.sh ]; then
    echo "Error: A/B OTA scripts not found"
    echo "This demo requires an A/B-enabled system"
    exit 1
fi

# Function to wait for user
wait_user() {
    echo ""
    read -p "Press ENTER to continue..."
    echo ""
}

# Step 1: Show current status
echo "STEP 1: Check Current System Status"
echo "------------------------------------"
/usr/local/bin/ab-slot-mgr.sh status
wait_user

# Step 2: Create a fake update package for demo
echo "STEP 2: Prepare Test Update Package"
echo "------------------------------------"
echo "In a real scenario, you would:"
echo "  1. Build a new system version"
echo "  2. Create update package: mk-ota-package.sh --board <board>"
echo "  3. Host it on a web server"
echo "  4. Download with: ota-update.sh update <url>"
echo ""
echo "For this demo, we'll simulate the installation step..."
wait_user

# Step 3: Show what happens during OTA
echo "STEP 3: OTA Update Process"
echo "--------------------------"
echo "The OTA update system will:"
echo "  ✓ Download update.tar.gz"
echo "  ✓ Verify checksum"
echo "  ✓ Extract rootfs.squashfs, Image, initramfs"
echo "  ✓ Copy to BOOT partition as rootfs-<inactive_slot>.squashfs"
echo "  ✓ Mark inactive slot as pending"
echo ""
echo "Your current system remains completely untouched!"
echo ""
INACTIVE=$(/usr/local/bin/ab-slot-mgr.sh get-inactive)
echo "Update would be installed to slot: $INACTIVE"
wait_user

# Step 4: Explain reboot process
echo "STEP 4: Reboot and Slot Switch"
echo "-------------------------------"
echo "After reboot, the initramfs will:"
echo "  1. Read A/B metadata from overlay partition"
echo "  2. See that there's a pending slot"
echo "  3. Switch to the pending slot"
echo "  4. Increment boot counter"
echo "  5. Mount rootfs-${INACTIVE}.squashfs"
echo "  6. Switch to the new system"
wait_user

# Step 5: Explain boot verification
echo "STEP 5: Boot Verification"
echo "-------------------------"
echo "After the system boots:"
echo "  • Boot counter starts at 1"
echo "  • System must reach multi-user.target"
echo "  • ab-boot-verify.service marks boot as successful"
echo "  • Boot counter resets to 0"
echo ""
echo "If boot fails (crash, hang, etc.):"
echo "  • On next boot, counter increments (2, 3...)"
echo "  • After 3 failed attempts, automatic fallback"
echo "  • System reverts to previous slot"
wait_user

# Step 6: Show slot information
echo "STEP 6: Detailed Slot Information"
echo "----------------------------------"
ACTIVE=$(/usr/local/bin/ab-slot-mgr.sh get-active)
echo "Active Slot ($ACTIVE):"
/usr/local/bin/ab-slot-mgr.sh get-slot-info $ACTIVE
echo ""
echo "Inactive Slot ($INACTIVE):"
/usr/local/bin/ab-slot-mgr.sh get-slot-info $INACTIVE
wait_user

# Step 7: Rollback
echo "STEP 7: Manual Rollback"
echo "-----------------------"
echo "If you want to revert to the previous slot:"
echo "  $ ota-update.sh rollback"
echo "  $ systemctl reboot"
echo ""
echo "This is useful if:"
echo "  • You notice issues after update"
echo "  • You want to test the old version"
echo "  • You need to debug a problem"
wait_user

# Step 8: Complete workflow example
echo "STEP 8: Complete Workflow Example"
echo "----------------------------------"
cat <<'EOF'
# Real-world OTA update workflow:

# 1. Build new version
cd output.h700
make
cd /path/to/TiniLinux

# 2. Create OTA package
./board/common/ota/mk-ota-package.sh --board h700

# 3. Upload to server
scp ota-packages/tinilinux-h700-*.tar.gz user@server:/var/www/updates/

# 4. On device, install update
ssh root@device
ota-update.sh update http://server/updates/tinilinux-h700-20260107.tar.gz

# Output:
# ===================================
# OTA Update Ready
# ===================================
# Slot b will be activated on next reboot.
# If the new system fails to boot, it will automatically
# fall back to the current slot after 3 attempts.
#
# To reboot now: systemctl reboot
# To cancel update: ab-slot-mgr.sh set-pending ''
# ===================================

# 5. Reboot to apply
systemctl reboot

# 6. After reboot, verify
ab-slot-mgr.sh status
# Active slot: b
# Boot successful: 1

# 7. If there's a problem, rollback
ota-update.sh rollback
systemctl reboot

EOF
wait_user

# Summary
echo "================================================"
echo "  Demo Complete!"
echo "================================================"
echo ""
echo "Key Takeaways:"
echo "  ✓ A/B OTA provides safe, seamless updates"
echo "  ✓ Automatic fallback protects against bad updates"
echo "  ✓ User data is never touched during updates"
echo "  ✓ Updates are atomic - old system untouched until reboot"
echo "  ✓ Can test updates and rollback if needed"
echo ""
echo "Documentation:"
echo "  • docs/AB-OTA-QUICKSTART.md - Quick reference"
echo "  • docs/AB-OTA-SYSTEM.md - Complete documentation"
echo "  • board/common/ota/README.md - Script documentation"
echo ""
echo "Try it yourself:"
echo "  1. Build A/B image: make imgab"
echo "  2. Test in QEMU: make runqemuab"
echo "  3. Check status: ab-slot-mgr.sh status"
echo ""
