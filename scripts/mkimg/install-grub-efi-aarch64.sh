#!/usr/bin/env bash
set -euo pipefail

# sudo dpkg --add-architecture arm64
# sudo apt update
# sudo apt install grub-efi-arm64-bin:arm64


IMG="${1:-tinilinux-qemu_aarch64.img}"

MNT_EFI="/mnt/vda1"

echo "[+] Attaching loop device..."
LOOP=$(sudo losetup --find --show -Pf "$IMG")
echo "    -> $LOOP"

EFI_DEV="${LOOP}p1"

echo "[+] Creating mount points..."
sudo mkdir -p "$MNT_EFI"

echo "[+] Mounting partitions..."
sudo mount "$EFI_DEV" "$MNT_EFI"

echo "[+] Installing GRUB (ARM64 EFI)..."
sudo grub-install \
  --target=arm64-efi \
  --efi-directory="$MNT_EFI" \
  --boot-directory="$MNT_EFI" \
  --removable \
  --no-nvram

echo "[+] Writing grub.cfg..."
sudo mkdir -p "$MNT_EFI/grub"
sudo tee "$MNT_EFI/grub/grub.cfg" > /dev/null <<'EOF'
set timeout=1
set default=0

menuentry "TiniLinux (aarch64)" {
    linux /Image console=ttyAMA0 console=tty1 video=640x480 splash loglevel=3 bootpart=/dev/vda1 root=root.img overlayfs=/dev/vda2
    initrd /initrd.img
}
EOF

echo "[+] Syncing..."
sync

echo "[+] Unmounting..."
sudo umount "$MNT_EFI" || true

echo "[+] Detaching loop device..."
sudo losetup -d "$LOOP"

echo "[+] Done! Your disk image is now bootable with GRUB + UEFI."
