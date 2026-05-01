#!/bin/bash
set -e

OPTIONAL_MODES=("$@")
for OPT in "${OPTIONAL_MODES[@]}"; do
    case "$OPT" in
        nographic) MODE_DISPLAY="nographic" ;;
        gui)       MODE_DISPLAY="gui"       ;;
        squashfs)  MODE_ROOT="squashfs"     ;;
        rootrw)    MODE_ROOT="rootrw"       ;;
        rootnfs)   MODE_ROOT="rootnfs"      ;;
        efi)       MODE_BOOT="efi"          ;;
        *) echo "Error: Unknown mode '$OPT'"; exit 1 ;;
    esac
done

disk=$(find . -name "tinilinux-*.img" -exec basename {} \;)

# --- Common args ---
QEMU_BASE=(
    qemu-system-aarch64
    -M virt -cpu cortex-a53 -smp 2 -m 1024M
    -drive file=$disk,if=none,format=raw,id=hd0
    -device virtio-blk-device,drive=hd0
    -netdev user,id=eth0
    -device virtio-net-device,netdev=eth0
    -device virtio-keyboard-pci
    -device virtio-mouse-pci
)

if [ "${MODE_BOOT:-}" = "efi" ]; then
    QEMU_BASE+=(
    -bios /usr/share/qemu-efi-aarch64/QEMU_EFI.fd
    )
else
    QEMU_BASE+=(
    -kernel Image
    -initrd initrd.img
    )
fi

# --- Resolve display mode ---
DISPLAY_APPEND="console=ttyAMA0"
DISPLAY_EXTRA=(    
    -serial mon:stdio
)
case "$MODE_DISPLAY" in
    nographic)
        DISPLAY_APPEND+=""
        DISPLAY_EXTRA+=(
            -nographic
        )
        ;;
    gui)
        DISPLAY_APPEND+=" console=tty1 video=640x480 splash"
        DISPLAY_EXTRA+=(
            -device virtio-gpu-gl-pci,xres=640,yres=480
            -display gtk,gl=on
        )
        ;;
esac

# --- Resolve root mode ---
case "$MODE_ROOT" in
    squashfs)
        ROOT_APPEND="loglevel=3 bootpart=/dev/vda1 root=root.img overlayfs=/dev/vda2"
        ;;
    rootrw)
        ROOT_APPEND="loglevel=3 root=/dev/vda2 fsck.repair=yes"
        ;;
    rootnfs)
        ROOT_APPEND="loglevel=3 root=nfs:10.0.2.2:/srv/nfs/qemu_development_rootfs networkconf=dhcp"
        ;;
esac

# --- Assemble kernel cmdline ---
APPEND="$(echo "${ROOT_APPEND} ${DISPLAY_APPEND}" | xargs)"

EXTRA=(
-virtfs local,path=/tmp,mount_tag=share,security_model=mapped-xattr
)

echo ""
if [ "${MODE_BOOT:-}" = "efi" ]; then
    echo "${QEMU_BASE[@]} ${DISPLAY_EXTRA[@]} ${EXTRA[@]}"
    echo ""
    exec ${QEMU_BASE[@]} ${DISPLAY_EXTRA[@]} ${EXTRA[@]}
else
    echo "${QEMU_BASE[@]} ${DISPLAY_EXTRA[@]} ${EXTRA[@]} -append \"$APPEND\""
    echo ""
    exec ${QEMU_BASE[@]} ${DISPLAY_EXTRA[@]} ${EXTRA[@]} -append "$APPEND"
fi


