include $(sort $(wildcard $(BR2_EXTERNAL_TiniLinux_PATH)/package/*/*.mk))

img:
	cd $(BR2_EXTERNAL_TiniLinux_PATH); \
	if [ -f $(BINARIES_DIR)/rootfs.squashfs ]; then \
		BOARD=$$(basename $(BR2_DEFCONFIG) _defconfig) $(BR2_EXTERNAL_TiniLinux_PATH)/board/common/mk-flashable-img-squashfs-rootless.sh; \
	else \
		BOARD=$$(basename $(BR2_DEFCONFIG) _defconfig) $(BR2_EXTERNAL_TiniLinux_PATH)/board/common/mk-flashable-img-rootless.sh; \
	fi

flash:
	cd $(BR2_EXTERNAL_TiniLinux_PATH); \
	BOARD=$$(basename $(BR2_DEFCONFIG) _defconfig) $(BR2_EXTERNAL_TiniLinux_PATH)/board/common/flash-to-sdcard.sh

clean-target:
	rm -rf $(BASE_TARGET_DIR) && find $(BUILD_DIR) -name ".stamp_target_installed" -delete && rm -f $(BUILD_DIR)/host-gcc-final-*/.stamp_host_installed

run-qemu:
	BOARD=$$(basename $(BR2_DEFCONFIG) _defconfig); \
	ARCH=$${BOARD#*_}; ARCH=$${ARCH#*_}; ARCH=$${ARCH%%_*}; \
	echo -e "\n\nStarting QEMU for board $$BOARD | arch $$ARCH ..."; \
	cd $(CONFIG_DIR); \
	./host/bin/qemu-system-$$ARCH -M virt -cpu cortex-a53 -nographic -smp 1 \
		-kernel images/Image \
		-initrd images/initramfs \
		-append "rootwait squashfspart=/dev/vda1 squashfsroot=rootfs.squashfs overlayfs=/dev/vda2 console=ttyAMA0" \
		-netdev user,id=eth0 \
		-device virtio-net-device,netdev=eth0 \
		-drive file=images/tinilinux-$$BOARD.img,if=none,format=raw,id=hd0 \
		-device virtio-blk-device,drive=hd0

run-qemu-gui:
	BOARD=$$(basename $(BR2_DEFCONFIG) _defconfig); \
	ARCH=$${BOARD#*_}; ARCH=$${ARCH#*_}; ARCH=$${ARCH%%_*}; \
	echo -e "\n\nStarting QEMU for board $$BOARD | arch $$ARCH ..."; \
	cd $(CONFIG_DIR); \
	qemu-system-aarch64 -M virt -cpu cortex-a53 -smp 2 -m 1G \
		-kernel images/Image \
		-initrd images/initramfs \
		-append "rootwait squashfspart=/dev/vda1 squashfsroot=rootfs.squashfs overlayfs=/dev/vda2 console=tty1" \
		-netdev user,id=eth0 \
		-device virtio-net-device,netdev=eth0 \
		-drive file=images/tinilinux-pc_qemu_aarch64_virt.img,if=none,format=raw,id=hd0 \
		-device virtio-blk-device,drive=hd0 \
		-device virtio-gpu-gl-pci,xres=640,yres=480 -display gtk,gl=on \
		-device virtio-keyboard-pci \
		-device virtio-mouse-pci
