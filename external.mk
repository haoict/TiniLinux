include $(sort $(wildcard $(BR2_EXTERNAL_TiniLinux_PATH)/package/*/*.mk))

img:
	cd $(BR2_EXTERNAL_TiniLinux_PATH); \
	BOARD=$(BR2_DEFCONFIG); \
	if [ -f $(BINARIES_DIR)/rootfs.squashfs ]; then \
		BOARD=$$(basename $$BOARD _defconfig) $(BR2_EXTERNAL_TiniLinux_PATH)/board/common/mk-flashable-img-squashfs-rootless.sh; \
	else \
		BOARD=$$(basename $$BOARD _defconfig) $(BR2_EXTERNAL_TiniLinux_PATH)/board/common/mk-flashable-img-rootless.sh; \
	fi

clean-target:
	rm -rf $(BASE_TARGET_DIR) && find $(BUILD_DIR) -name ".stamp_target_installed" -delete && rm -f $(BUILD_DIR)/host-gcc-final-*/.stamp_host_installed
