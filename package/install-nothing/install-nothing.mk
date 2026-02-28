################################################################################
#
# install-nothing
#
################################################################################

INSTALL_NOTHING_VERSION = 0.5.0
INSTALL_NOTHING_SITE = https://github.com/buyukakyuz/install-nothing/releases/download/v${INSTALL_NOTHING_VERSION}

ifeq ($(findstring x86_64,$(BR2_DEFCONFIG)),x86_64)
	INSTALL_NOTHING_SOURCE = install-nothing-linux-x86_64
else
	INSTALL_NOTHING_SOURCE = install-nothing-linux-aarch64
endif

INSTALL_NOTHING_INSTALL_TARGET = YES

define INSTALL_NOTHING_EXTRACT_CMDS
	cp $(DL_DIR)/install-nothing/${INSTALL_NOTHING_SOURCE} $(@D)/${INSTALL_NOTHING_SOURCE}
endef

define INSTALL_NOTHING_INSTALL_TARGET_CMDS
	mkdir -p $(TARGET_DIR)/usr/local/bin
	cp $(@D)/${INSTALL_NOTHING_SOURCE} $(TARGET_DIR)/usr/local/bin/${INSTALL_NOTHING_SOURCE}
	chmod +x $(TARGET_DIR)/usr/local/bin/${INSTALL_NOTHING_SOURCE}
endef

$(eval $(generic-package))
