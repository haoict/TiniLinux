include $(sort $(wildcard $(BR2_EXTERNAL_TiniLinux_PATH)/package/*/*.mk))

BOARD=$(shell basename $(BR2_DEFCONFIG) _defconfig)

img:
	cd $(CONFIG_DIR); \
	if [ -f $(BINARIES_DIR)/rootfs.tar ]; then \
		BOARD=$(BOARD) $(BR2_EXTERNAL_TiniLinux_PATH)/scripts/mkimg/mk-flashable-img-rootrw-rootless.sh; \
	else \
		BOARD=$(BOARD) $(BR2_EXTERNAL_TiniLinux_PATH)/scripts/mkimg/mk-flashable-img-squashfs-rootless.sh; \
	fi

flash:
	cd $(CONFIG_DIR); \
	BOARD=$(BOARD) $(BR2_EXTERNAL_TiniLinux_PATH)/scripts/mkimg/flash-to-sdcard.sh

cleantarget:
	rm -rf $(BASE_TARGET_DIR); \
	find $(BUILD_DIR) -name ".stamp_target_installed" -delete; \
	rm -f $(BUILD_DIR)/host-gcc-final-*/.stamp_host_installed; \
	rm -rf $(CONFIG_DIR)/per-package/skeleton-init-systemd; \
	rm -rf $(CONFIG_DIR)/build/skeleton-init-systemd;

savefconf:
	cd $(CONFIG_DIR) && $(BR2_EXTERNAL_TiniLinux_PATH)/scripts/save-fragment-defconfig.sh

runqemu:
	cd $(BINARIES_DIR); \
	qemu-system-aarch64 -M virt -cpu cortex-a53 -smp 1 -m 1G \
		-kernel Image \
		-initrd initramfs \
		-append "bootpart=/dev/vda1 squashfsimg=rootfs.squashfs overlayfs=/dev/vda2 console=ttyAMA0" \
		-netdev user,id=eth0 \
		-device virtio-net-device,netdev=eth0 \
		-drive file=tinilinux-$(BOARD).img,if=none,format=raw,id=hd0 \
		-device virtio-blk-device,drive=hd0 \
		-nographic

runqemugui:
	cd $(BINARIES_DIR); \
	qemu-system-aarch64 -M virt -cpu cortex-a53 -smp 2 -m 1G \
		-kernel Image \
		-initrd initramfs \
		-append "bootpart=/dev/vda1 squashfsimg=rootfs.squashfs overlayfs=/dev/vda2 console=ttyAMA0 video=640x480" \
		-netdev user,id=eth0 \
		-device virtio-net-device,netdev=eth0 \
		-drive file=tinilinux-$(BOARD).img,if=none,format=raw,id=hd0 \
		-device virtio-blk-device,drive=hd0 \
		-device virtio-gpu-gl-pci,xres=640,yres=480 -display gtk,gl=on \
		-device virtio-keyboard-pci \
		-device virtio-mouse-pci \
		-serial mon:stdio

runqemurootrw:
	cd $(BINARIES_DIR); \
	qemu-system-aarch64 -M virt -cpu cortex-a53 -smp 1 -m 1G \
		-kernel Image \
		-initrd initramfs \
		-append "root=/dev/vda2 fsck.repair=yes console=ttyAMA0" \
		-netdev user,id=eth0 \
		-device virtio-net-device,netdev=eth0 \
		-virtfs local,path=/home/haoict/Documents/filebrowser/srv/public,mount_tag=share,security_model=mapped-xattr \
		-drive file=tinilinux-$(BOARD).img,if=none,format=raw,id=hd0 \
		-device virtio-blk-device,drive=hd0 \
		-nographic

runqemurootnfs:
	cd $(BINARIES_DIR); \
	qemu-system-aarch64 -M virt -cpu cortex-a53 -smp 1 -m 1G \
		-kernel Image \
		-initrd initramfs \
		-append "root=nfs:10.0.2.2:/home/haoict/Documents/nfs/shared ip=dhcp console=ttyAMA0" \
		-netdev user,id=eth0 \
		-device virtio-net-device,netdev=eth0 \
		-drive file=tinilinux-$(BOARD).img,if=none,format=raw,id=hd0 \
		-device virtio-blk-device,drive=hd0 \
		-nographic

liveiso:
	cd $(BINARIES_DIR); \
	rm -rf iso; \
	mkdir -p iso; \
	cp -r $(BR2_EXTERNAL_TiniLinux_PATH)/board/pc_x86_64_efi_liveboot/BOOT iso/boot; \
	cp bzImage iso/boot; \
	cp initramfs iso/boot; \
	cp rootfs.squashfs iso/boot; \
	grub-mkrescue -o tinilinux-x86_64-liveboot.iso iso

runqemuiso:
	cd $(BINARIES_DIR); \
	qemu-system-x86_64 -enable-kvm -cpu host -smp 2 -m 2G \
		-drive if=pflash,format=raw,readonly=on,file=/usr/share/ovmf/OVMF.fd \
		-boot d \
		-cdrom tinilinux-x86_64-liveboot.iso \
		-device virtio-gpu-gl,xres=640,yres=480 -display gtk,gl=on \
		-device virtio-keyboard \
		-device virtio-mouse \
		-vga none

# unused
runqemux64:
	cd $(BINARIES_DIR); \
	qemu-system-x86_64 -M q35 -smp 1 -m 1G \
		-kernel bzImage \
		-initrd initramfs \
		-append "bootpart=/dev/vda1 squashfsimg=rootfs.squashfs overlayfs=/dev/vda2 console=ttyS0" \
		-netdev user,id=eth0 \
		-device virtio-net-pci,netdev=eth0 \
		-drive file=tinilinux-$(BOARD).img,if=none,format=raw,id=hd0 \
		-device virtio-blk-pci,drive=hd0 \
		-nographic \
		-serial mon:stdio
