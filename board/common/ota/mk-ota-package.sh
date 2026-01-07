#!/bin/bash
#
# TiniLinux OTA Package Builder
# Creates OTA update packages from Buildroot output
#

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BOARD="${BOARD:-}"
OUTPUT_DIR=""
PACKAGE_NAME=""
INCLUDE_KERNEL=1
INCLUDE_INITRAMFS=1
INCLUDE_DTB=1

usage() {
    cat <<EOF
TiniLinux OTA Package Builder

Usage: $0 [OPTIONS]

Options:
  -b, --board BOARD          Board name (required)
  -o, --output DIR           Output directory (default: ./ota-packages)
  -n, --name NAME            Package name (default: tinilinux-<board>-<date>)
  --no-kernel                Don't include kernel
  --no-initramfs             Don't include initramfs
  --no-dtb                   Don't include device trees
  -h, --help                 Show this help message

Example:
  $0 --board h700
  $0 --board pc_qemu_aarch64_virt --output /tmp/ota

The script will create a .tar.gz package containing:
  - rootfs.squashfs (always included)
  - Image (kernel, optional)
  - initramfs (optional)
  - dtb/ (device trees, optional)
  - manifest.txt (metadata)
  - checksum.sha256 (package checksum)

EOF
    exit 1
}

# Parse arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        -b|--board)
            BOARD="$2"
            shift 2
            ;;
        -o|--output)
            OUTPUT_DIR="$2"
            shift 2
            ;;
        -n|--name)
            PACKAGE_NAME="$2"
            shift 2
            ;;
        --no-kernel)
            INCLUDE_KERNEL=0
            shift
            ;;
        --no-initramfs)
            INCLUDE_INITRAMFS=0
            shift
            ;;
        --no-dtb)
            INCLUDE_DTB=0
            shift
            ;;
        -h|--help)
            usage
            ;;
        *)
            echo "Unknown option: $1"
            usage
            ;;
    esac
done

# Validate board
if [ -z "$BOARD" ]; then
    echo "Error: Board name is required"
    usage
fi

# Set defaults
if [ -z "$OUTPUT_DIR" ]; then
    OUTPUT_DIR="$SCRIPT_DIR/../ota-packages"
fi

if [ -z "$PACKAGE_NAME" ]; then
    PACKAGE_NAME="tinilinux-${BOARD}-$(date +%Y%m%d-%H%M%S)"
fi

# Check if output directory exists
IMAGES_DIR="$SCRIPT_DIR/../output.${BOARD}/images"
if [ ! -d "$IMAGES_DIR" ]; then
    echo "Error: Images directory not found: $IMAGES_DIR"
    echo "Have you built the system? (make in output.$BOARD)"
    exit 1
fi

# Check if rootfs.squashfs exists
if [ ! -f "$IMAGES_DIR/rootfs.squashfs" ]; then
    echo "Error: rootfs.squashfs not found in $IMAGES_DIR"
    exit 1
fi

echo "=========================================="
echo "TiniLinux OTA Package Builder"
echo "=========================================="
echo "Board:       $BOARD"
echo "Package:     $PACKAGE_NAME"
echo "Output:      $OUTPUT_DIR"
echo ""

# Create output and temporary directories
mkdir -p "$OUTPUT_DIR"
WORK_DIR=$(mktemp -d)
trap "rm -rf $WORK_DIR" EXIT

echo "[1/5] Preparing package contents..."

# Copy rootfs.squashfs (always required)
echo "  ✓ Adding rootfs.squashfs"
cp "$IMAGES_DIR/rootfs.squashfs" "$WORK_DIR/"

# Copy kernel if requested
if [ "$INCLUDE_KERNEL" -eq 1 ] && [ -f "$IMAGES_DIR/Image" ]; then
    echo "  ✓ Adding kernel (Image)"
    cp "$IMAGES_DIR/Image" "$WORK_DIR/"
elif [ "$INCLUDE_KERNEL" -eq 1 ]; then
    echo "  ⚠ Kernel not found, skipping"
fi

# Copy initramfs if requested
if [ "$INCLUDE_INITRAMFS" -eq 1 ] && [ -f "$IMAGES_DIR/initramfs" ]; then
    echo "  ✓ Adding initramfs"
    cp "$IMAGES_DIR/initramfs" "$WORK_DIR/"
elif [ "$INCLUDE_INITRAMFS" -eq 1 ]; then
    echo "  ⚠ Initramfs not found, skipping"
fi

# Copy device trees if requested
if [ "$INCLUDE_DTB" -eq 1 ]; then
    if [[ "${BOARD}" == "rgb30"* ]] && [ -d "$IMAGES_DIR/rockchip" ]; then
        echo "  ✓ Adding device trees (rockchip)"
        cp -r "$IMAGES_DIR/rockchip" "$WORK_DIR/dtb"
        if [ -d "$IMAGES_DIR/rk3566-dtbo" ]; then
            cp "$IMAGES_DIR/rk3566-dtbo"/*.dtbo "$WORK_DIR/dtb/" 2>/dev/null || true
        fi
    elif [[ "${BOARD}" == "h700"* ]] && [ -d "$IMAGES_DIR/allwinner" ]; then
        echo "  ✓ Adding device trees (allwinner)"
        cp -r "$IMAGES_DIR/allwinner" "$WORK_DIR/dtb"
    else
        echo "  ⚠ No device trees found for board $BOARD"
    fi
fi

echo ""
echo "[2/5] Generating manifest..."

# Create manifest
MANIFEST_FILE="$WORK_DIR/manifest.txt"
cat > "$MANIFEST_FILE" <<EOF
TiniLinux OTA Update Package
============================
Board: $BOARD
Package: $PACKAGE_NAME
Created: $(date -u +"%Y-%m-%d %H:%M:%S UTC")
Build ID: $(cat $IMAGES_DIR/../target/etc/os-release 2>/dev/null | grep BUILD_ID | cut -d'=' -f2 || echo "unknown")

Contents:
EOF

# List package contents with sizes
for file in $(find "$WORK_DIR" -type f | sort); do
    filename=$(basename "$file")
    size=$(stat -f%z "$file" 2>/dev/null || stat -c%s "$file")
    size_mb=$(echo "scale=2; $size / 1024 / 1024" | bc)
    echo "  - $filename (${size_mb} MB)" >> "$MANIFEST_FILE"
done

cat "$MANIFEST_FILE"

echo ""
echo "[3/5] Creating package archive..."

cd "$WORK_DIR"
tar -czf "${OUTPUT_DIR}/${PACKAGE_NAME}.tar.gz" .
cd - > /dev/null

echo "  ✓ Package created: ${OUTPUT_DIR}/${PACKAGE_NAME}.tar.gz"

echo ""
echo "[4/5] Generating checksums..."

# Generate SHA256 checksum
CHECKSUM_FILE="${OUTPUT_DIR}/${PACKAGE_NAME}.sha256"
cd "$OUTPUT_DIR"
sha256sum "${PACKAGE_NAME}.tar.gz" > "$CHECKSUM_FILE"
cd - > /dev/null

echo "  ✓ Checksum: $(cat $CHECKSUM_FILE)"

echo ""
echo "[5/5] Finalizing..."

# Get final package size
PACKAGE_SIZE=$(stat -f%z "${OUTPUT_DIR}/${PACKAGE_NAME}.tar.gz" 2>/dev/null || \
               stat -c%s "${OUTPUT_DIR}/${PACKAGE_NAME}.tar.gz")
PACKAGE_SIZE_MB=$(echo "scale=2; $PACKAGE_SIZE / 1024 / 1024" | bc)

echo ""
echo "=========================================="
echo "OTA Package Created Successfully!"
echo "=========================================="
echo ""
echo "Package:  ${PACKAGE_NAME}.tar.gz"
echo "Size:     ${PACKAGE_SIZE_MB} MB"
echo "Location: ${OUTPUT_DIR}/"
echo ""
echo "Files:"
echo "  - ${PACKAGE_NAME}.tar.gz    (OTA package)"
echo "  - ${PACKAGE_NAME}.sha256    (checksum)"
echo ""
echo "To deploy this update:"
echo "  1. Host the .tar.gz on a web server"
echo "  2. On device: ota-update.sh update http://yourserver/${PACKAGE_NAME}.tar.gz"
echo ""
echo "Or for testing:"
echo "  scp ${PACKAGE_NAME}.tar.gz root@device:/tmp/"
echo "  ssh root@device 'cd /tmp && tar -xzf ${PACKAGE_NAME}.tar.gz && ota-update.sh install && ota-update.sh activate'"
echo ""
