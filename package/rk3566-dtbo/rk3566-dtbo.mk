################################################################################
# RK3566 DTBO
################################################################################

RK3566_DTBO_VERSION = 1.0
RK3566_DTBO_SITE = $(BR2_EXTERNAL_TiniLinux_PATH)/package/rk3566-dtbo
RK3566_DTBO_SITE_METHOD = local

define RK3566_DTBO_BUILD_CMDS
	for f in $(@D)/*.dts; do \
		base=$$(basename $$f .dts); \
		$(HOST_DIR)/bin/dtc -@ -I dts -O dtb \
			-o $(@D)/$$base.dtbo $$f; \
	done
endef

define RK3566_DTBO_INSTALL_TARGET_CMDS
	$(INSTALL) -d $(BINARIES_DIR)/rk3566-dtbo
	$(INSTALL) -m 0644 $(@D)/*.dtbo $(BINARIES_DIR)/rk3566-dtbo/
endef

$(eval $(generic-package))