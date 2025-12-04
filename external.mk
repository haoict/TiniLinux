include $(sort $(wildcard $(BR2_EXTERNAL_TiniLinux_PATH)/package/*/*.mk))

img:
	cd $(BR2_EXTERNAL_TiniLinux_PATH); \
	BOARD=$(BR2_DEFCONFIG); \
	BOARD=$$(basename $$BOARD _defconfig) $(BR2_EXTERNAL_TiniLinux_PATH)/board/common/mk-flashable-img-rootless.sh

img-squashfs:
	cd $(BR2_EXTERNAL_TiniLinux_PATH); \
	BOARD=$(BR2_DEFCONFIG); \
	BOARD=$$(basename $$BOARD _defconfig) $(BR2_EXTERNAL_TiniLinux_PATH)/board/common/mk-flashable-img-squashfs-rootless.sh

clean-target:
	rm -rf $(BASE_TARGET_DIR) && find $(BUILD_DIR) -name ".stamp_target_installed" -delete && rm -f $(BUILD_DIR)/host-gcc-final-*/.stamp_host_installed
