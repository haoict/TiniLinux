################################################################################
#
# inputplumber
#
################################################################################

INPUTPLUMBER_VERSION = 0.84.0
INPUTPLUMBER_SITE = https://github.com/ShadowBlip/InputPlumber/releases/download/v${INPUTPLUMBER_VERSION}
ifeq ($(findstring x86_64,$(BR2_DEFCONFIG)),x86_64)
	INPUTPLUMBER_SOURCE = inputplumber-x86_64.tar.gz
else
	INPUTPLUMBER_SOURCE = inputplumber-aarch64.tar.gz
endif

INPUTPLUMBER_INSTALL_TARGET = YES

define INPUTPLUMBER_INSTALL_TARGET_CMDS
	cp -r $(@D)/usr/* $(TARGET_DIR)/usr/
	chmod +x $(TARGET_DIR)/usr/bin/inputplumber
	cp -r $(BR2_EXTERNAL_TiniLinux_PATH)/package/inputplumber/custom_usr/* $(TARGET_DIR)/usr/
	chmod +x $(TARGET_DIR)/usr/lib/inputplumber/setfacl-shim/setfacl
endef

$(eval $(generic-package))
