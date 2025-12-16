To run:
```bash
cd output.pc_qemu_aarch64_virt

# run with rootfs (ext4)
./host/bin/qemu-system-aarch64 -M virt -cpu cortex-a53 -nographic -smp 1 -kernel images/Image -initrd images/initramfs -append "rootwait root=/dev/vda console=ttyAMA0" -netdev user,id=eth0 -device virtio-net-device,netdev=eth0 -drive file=images/rootfs.ext4,if=none,format=raw,id=hd0 -device virtio-blk-device,drive=hd0

# run with squashfs
./host/bin/qemu-system-aarch64 -M virt -cpu cortex-a53 -nographic -smp 1 -kernel images/Image -initrd images/initramfs -append "rootwait squashfspart=/dev/vda1 squashfsroot=rootfs.squashfs overlayfs=/dev/vda2 console=ttyAMA0" -netdev user,id=eth0 -device virtio-net-device,netdev=eth0 -drive file=images/tinilinux-pc_qemu_aarch64_virt.img,if=none,format=raw,id=hd0 -device virtio-blk-device,drive=hd0
```