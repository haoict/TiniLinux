################################################################################
#
# Bluetooth TUI
#
################################################################################

BLUETUI_VERSION = v0.8.1
BLUETUI_SITE = https://github.com/pythops/bluetui/releases/download/${BLUETUI_VERSION}

ifeq ($(findstring x86_64,$(BR2_DEFCONFIG)),x86_64)
	BLUETUI_SOURCE = bluetui-x86_64-linux-musl
else
	BLUETUI_SOURCE = bluetui-aarch64-linux-musl
endif

BLUETUI_INSTALL_TARGET = YES

define BLUETUI_EXTRACT_CMDS
	cp $(DL_DIR)/bluetui/${BLUETUI_SOURCE} $(@D)/${BLUETUI_SOURCE}
endef

define BLUETUI_INSTALL_TARGET_CMDS
	mkdir -p $(TARGET_DIR)/usr/local/bin
	cp $(@D)/${BLUETUI_SOURCE} $(TARGET_DIR)/usr/local/bin/bluetui
	chmod +x $(TARGET_DIR)/usr/local/bin/bluetui
endef

$(eval $(generic-package))
