include $(sort $(wildcard $(BR2_EXTERNAL_TiniLinux_PATH)/package/*/*.mk))

BOARD=$(shell basename $(BR2_DEFCONFIG) _defconfig)

img:
	cd $(CONFIG_DIR); \
	if [ -f $(BINARIES_DIR)/rootfs.tar ]; then \
		BOARD=$(BOARD) $(BR2_EXTERNAL_TiniLinux_PATH)/board/common/mk-flashable-img-rootrw-rootless.sh; \
	else \
		BOARD=$(BOARD) $(BR2_EXTERNAL_TiniLinux_PATH)/board/common/mk-flashable-img-squashfs-rootless.sh; \
	fi

flash:
	cd $(CONFIG_DIR); \
	BOARD=$(BOARD) $(BR2_EXTERNAL_TiniLinux_PATH)/board/common/flash-to-sdcard.sh

cleantarget:
	rm -rf $(BASE_TARGET_DIR) && find $(BUILD_DIR) -name ".stamp_target_installed" -delete && rm -f $(BUILD_DIR)/host-gcc-final-*/.stamp_host_installed

savefconf:
	cd $(CONFIG_DIR) && $(BR2_EXTERNAL_TiniLinux_PATH)/board/common/save-fragment-defconfig.sh

runqemu:
	cd $(BINARIES_DIR); \
	qemu-system-aarch64 -M virt -cpu cortex-a53 -smp 1 -m 1G \
		-kernel Image \
		-initrd initramfs \
		-append "rootwait bootpart=/dev/vda1 squashfsimg=rootfs.squashfs overlayfs=/dev/vda2 console=ttyAMA0" \
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
		-append "rootwait bootpart=/dev/vda1 squashfsimg=rootfs.squashfs overlayfs=/dev/vda2 console=ttyAMA0" \
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
		-append "rootwait root=/dev/vda2 console=ttyAMA0" \
		-netdev user,id=eth0 \
		-device virtio-net-device,netdev=eth0 \
		-drive file=tinilinux-$(BOARD).img,if=none,format=raw,id=hd0 \
		-device virtio-blk-device,drive=hd0 \
		-nographic

# unused
runqemux64:
	cd $(BINARIES_DIR); \
	qemu-system-x86_64 -M q35 -smp 1 -m 1G \
		-kernel bzImage \
		-initrd initramfs \
		-append "rootwait bootpart=/dev/vda1 squashfsimg=rootfs.squashfs overlayfs=/dev/vda2 console=ttyS0" \
		-netdev user,id=eth0 \
		-device virtio-net-pci,netdev=eth0 \
		-drive file=tinilinux-$(BOARD).img,if=none,format=raw,id=hd0 \
		-device virtio-blk-pci,drive=hd0 \
		-nographic \
		-serial mon:stdio
