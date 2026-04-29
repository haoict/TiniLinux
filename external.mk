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
	# make runqemu OPT="gui rootnfs"
	# refer to run-qemu-aarch64.sh for available options
	cd $(BINARIES_DIR); \
	$(BR2_EXTERNAL_TiniLinux_PATH)/scripts/run-qemu-aarch64.sh nographic squashfs $(OPT)
