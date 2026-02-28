################################################################################
# PI_ST7789V_240X240 DTBO
################################################################################

PI_ST7789V_240X240_DTBO_VERSION = 1.0
PI_ST7789V_240X240_DTBO_SITE = $(BR2_EXTERNAL_TiniLinux_PATH)/package/pi-st7789v-240x240-dtbo
PI_ST7789V_240X240_DTBO_SITE_METHOD = local
PI_ST7789V_240X240_DTBO_DEPENDENCIES = host-dtc

define PI_ST7789V_240X240_DTBO_BUILD_CMDS
	for f in $(@D)/*.dts; do \
		base=$$(basename $$f .dts); \
		$(HOST_DIR)/bin/dtc -@ -I dts -O dtb \
			-o $(@D)/$$base.dtbo $$f; \
	done

	cd $(@D) && python3 $(@D)/gen_firmware.py
endef

define PI_ST7789V_240X240_DTBO_INSTALL_TARGET_CMDS
	$(INSTALL) -m 0644 $(@D)/*.dtbo $(BINARIES_DIR)/rpi-firmware/overlays/
	$(INSTALL) -d $(TARGET_DIR)/lib/firmware/
	$(INSTALL) -m 0644 $(@D)/panel-mipi-dbi-spi.bin $(TARGET_DIR)/lib/firmware/
endef

$(eval $(generic-package))