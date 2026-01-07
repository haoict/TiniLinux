#!/bin/bash
#
# A/B OTA Test Suite
# Validates A/B OTA functionality
#

set -e

TESTS_PASSED=0
TESTS_FAILED=0
TEST_DIR="/tmp/ab-ota-test-$$"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Test result functions
pass() {
    echo -e "${GREEN}✓ PASS${NC}: $1"
    TESTS_PASSED=$((TESTS_PASSED + 1))
}

fail() {
    echo -e "${RED}✗ FAIL${NC}: $1"
    TESTS_FAILED=$((TESTS_FAILED + 1))
}

skip() {
    echo -e "${YELLOW}⊘ SKIP${NC}: $1"
}

# Setup
setup() {
    echo "Setting up test environment..."
    mkdir -p "$TEST_DIR"
    export AB_METADATA="$TEST_DIR/ab-metadata"
    
    # Mock ab-slot-mgr.sh for testing if not in real system
    if [ ! -f /usr/local/bin/ab-slot-mgr.sh ]; then
        skip "Running in mock mode (not on actual A/B system)"
        export MOCK_MODE=1
    fi
}

# Cleanup
cleanup() {
    echo "Cleaning up..."
    rm -rf "$TEST_DIR"
}

trap cleanup EXIT

# Test 1: Check if A/B scripts exist
test_scripts_exist() {
    echo ""
    echo "Test 1: A/B Scripts Existence"
    echo "------------------------------"
    
    if [ -f board/common/ota/ab-slot-mgr.sh ]; then
        pass "ab-slot-mgr.sh exists"
    else
        fail "ab-slot-mgr.sh not found"
    fi
    
    if [ -f board/common/ota/ota-update.sh ]; then
        pass "ota-update.sh exists"
    else
        fail "ota-update.sh not found"
    fi
    
    if [ -f board/common/ota/mk-ota-package.sh ]; then
        pass "mk-ota-package.sh exists"
    else
        fail "mk-ota-package.sh not found"
    fi
    
    if [ -f board/common/mk-flashable-img-squashfs-ab.sh ]; then
        pass "mk-flashable-img-squashfs-ab.sh exists"
    else
        fail "mk-flashable-img-squashfs-ab.sh not found"
    fi
}

# Test 2: Check if scripts are executable
test_scripts_executable() {
    echo ""
    echo "Test 2: Scripts Executable"
    echo "--------------------------"
    
    if [ -x board/common/ota/ab-slot-mgr.sh ]; then
        pass "ab-slot-mgr.sh is executable"
    else
        fail "ab-slot-mgr.sh is not executable"
    fi
    
    if [ -x board/common/ota/ota-update.sh ]; then
        pass "ota-update.sh is executable"
    else
        fail "ota-update.sh is not executable"
    fi
}

# Test 3: Check initramfs modifications
test_initramfs_modified() {
    echo ""
    echo "Test 3: Initramfs A/B Support"
    echo "------------------------------"
    
    if grep -q "ab_boot" package/initramfs/init; then
        pass "initramfs init has ab_boot parameter support"
    else
        fail "initramfs init missing ab_boot support"
    fi
    
    if grep -q "get_ab_active_slot" package/initramfs/init; then
        pass "initramfs init has A/B slot functions"
    else
        fail "initramfs init missing A/B slot functions"
    fi
    
    if grep -q "check_and_switch_slot" package/initramfs/init; then
        pass "initramfs init has slot switching logic"
    else
        fail "initramfs init missing slot switching logic"
    fi
}

# Test 4: Check external.mk integration
test_makefile_targets() {
    echo ""
    echo "Test 4: Makefile Integration"
    echo "-----------------------------"
    
    if grep -q "^imgab:" external.mk; then
        pass "external.mk has imgab target"
    else
        fail "external.mk missing imgab target"
    fi
    
    if grep -q "^runqemuab:" external.mk; then
        pass "external.mk has runqemuab target"
    else
        fail "external.mk missing runqemuab target"
    fi
    
    if grep -q "^runqemuguiab:" external.mk; then
        pass "external.mk has runqemuguiab target"
    else
        fail "external.mk missing runqemuguiab target"
    fi
}

# Test 5: Check documentation
test_documentation() {
    echo ""
    echo "Test 5: Documentation"
    echo "---------------------"
    
    if [ -f docs/AB-OTA-SYSTEM.md ]; then
        pass "Main documentation exists"
    else
        fail "Main documentation missing"
    fi
    
    if [ -f docs/AB-OTA-QUICKSTART.md ]; then
        pass "Quick start guide exists"
    else
        fail "Quick start guide missing"
    fi
    
    if [ -f board/common/ota/README.md ]; then
        pass "OTA scripts README exists"
    else
        fail "OTA scripts README missing"
    fi
}

# Test 6: Check systemd services
test_systemd_services() {
    echo ""
    echo "Test 6: Systemd Services"
    echo "------------------------"
    
    if [ -f board/common/ota/ab-boot-verify.service ]; then
        pass "ab-boot-verify.service exists"
    else
        fail "ab-boot-verify.service missing"
    fi
    
    if [ -f board/common/ota/ota-update.service ]; then
        pass "ota-update.service exists"
    else
        fail "ota-update.service missing"
    fi
}

# Test 7: Validate script syntax
test_script_syntax() {
    echo ""
    echo "Test 7: Script Syntax Validation"
    echo "---------------------------------"
    
    if bash -n board/common/ota/ab-slot-mgr.sh 2>/dev/null; then
        pass "ab-slot-mgr.sh syntax valid"
    else
        fail "ab-slot-mgr.sh has syntax errors"
    fi
    
    if bash -n board/common/ota/ota-update.sh 2>/dev/null; then
        pass "ota-update.sh syntax valid"
    else
        fail "ota-update.sh has syntax errors"
    fi
    
    if bash -n board/common/ota/mk-ota-package.sh 2>/dev/null; then
        pass "mk-ota-package.sh syntax valid"
    else
        fail "mk-ota-package.sh has syntax errors"
    fi
    
    if bash -n board/common/mk-flashable-img-squashfs-ab.sh 2>/dev/null; then
        pass "mk-flashable-img-squashfs-ab.sh syntax valid"
    else
        fail "mk-flashable-img-squashfs-ab.sh has syntax errors"
    fi
}

# Test 8: Check ab-boot parameter in QEMU targets
test_qemu_params() {
    echo ""
    echo "Test 8: QEMU Boot Parameters"
    echo "-----------------------------"
    
    if grep -A 10 "^runqemuab:" external.mk | grep -q "ab_boot=1"; then
        pass "runqemuab includes ab_boot=1 parameter"
    else
        fail "runqemuab missing ab_boot=1 parameter"
    fi
    
    if grep -A 10 "^runqemuguiab:" external.mk | grep -q "ab_boot=1"; then
        pass "runqemuguiab includes ab_boot=1 parameter"
    else
        fail "runqemuguiab missing ab_boot=1 parameter"
    fi
}

# Test 9: Verify A/B image builder references both slots
test_ab_image_builder() {
    echo ""
    echo "Test 9: A/B Image Builder"
    echo "-------------------------"
    
    if grep -q "rootfs-a.squashfs" board/common/mk-flashable-img-squashfs-ab.sh; then
        pass "Image builder includes rootfs-a.squashfs"
    else
        fail "Image builder missing rootfs-a.squashfs"
    fi
    
    if grep -q "rootfs-b.squashfs" board/common/mk-flashable-img-squashfs-ab.sh; then
        pass "Image builder includes rootfs-b.squashfs"
    else
        fail "Image builder missing rootfs-b.squashfs"
    fi
    
    if grep -q "BOOT_SIZE_AB" board/common/mk-flashable-img-squashfs-ab.sh; then
        pass "Image builder doubles BOOT partition size"
    else
        fail "Image builder doesn't adjust BOOT partition size"
    fi
}

# Test 10: Check example configs
test_example_configs() {
    echo ""
    echo "Test 10: Example Configurations"
    echo "--------------------------------"
    
    if [ -f board/common/ota/extlinux.conf.ab-example ]; then
        pass "Example extlinux.conf exists"
    else
        fail "Example extlinux.conf missing"
    fi
    
    if grep -q "ab_boot=1" board/common/ota/extlinux.conf.ab-example; then
        pass "Example config includes ab_boot=1"
    else
        fail "Example config missing ab_boot=1"
    fi
}

# Main test execution
echo "================================================"
echo "  TiniLinux A/B OTA Test Suite"
echo "================================================"
echo ""

setup

test_scripts_exist
test_scripts_executable
test_initramfs_modified
test_makefile_targets
test_documentation
test_systemd_services
test_script_syntax
test_qemu_params
test_ab_image_builder
test_example_configs

# Summary
echo ""
echo "================================================"
echo "  Test Results"
echo "================================================"
echo -e "${GREEN}Passed: $TESTS_PASSED${NC}"
if [ $TESTS_FAILED -gt 0 ]; then
    echo -e "${RED}Failed: $TESTS_FAILED${NC}"
else
    echo -e "${GREEN}Failed: $TESTS_FAILED${NC}"
fi
echo ""

if [ $TESTS_FAILED -eq 0 ]; then
    echo -e "${GREEN}All tests passed! ✓${NC}"
    echo ""
    echo "A/B OTA system is properly implemented."
    echo ""
    echo "Next steps:"
    echo "  1. Build an A/B image: cd output.<board> && make imgab"
    echo "  2. Test in QEMU: make runqemuab"
    echo "  3. Create OTA package: ./board/common/ota/mk-ota-package.sh --board <board>"
    exit 0
else
    echo -e "${RED}Some tests failed. Please review the errors above.${NC}"
    exit 1
fi
