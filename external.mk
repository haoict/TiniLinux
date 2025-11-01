include $(sort $(wildcard $(BR2_EXTERNAL_TiniLinux_PATH)/package/*/*.mk))

clean-target:
	rm -rf $(BASE_TARGET_DIR) && find $(BUILD_DIR) -name ".stamp_target_installed" -delete && rm -f $(BUILD_DIR)/host-gcc-final-*/.stamp_host_installed
