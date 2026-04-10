################################################################################
#
# fbgif
#
################################################################################

FBGIF_VERSION = 0.2.1
FBGIF_SITE = https://github.com/haoict/fbgif/releases/download/${FBGIF_VERSION}

ifeq ($(findstring x86_64,$(BR2_DEFCONFIG)),x86_64)
	FBGIF_SOURCE = fbgif-linux-x86_64
else
	FBGIF_SOURCE = fbgif-linux-aarch64
endif

FBGIF_INSTALL_TARGET = YES

define FBGIF_EXTRACT_CMDS
	cp $(DL_DIR)/fbgif/${FBGIF_SOURCE} $(@D)/${FBGIF_SOURCE}
endef

define FBGIF_INSTALL_TARGET_CMDS
	mkdir -p $(TARGET_DIR)/usr/local/bin
	cp $(@D)/${FBGIF_SOURCE} $(TARGET_DIR)/usr/local/bin/${FBGIF_SOURCE}
	chmod +x $(TARGET_DIR)/usr/local/bin/${FBGIF_SOURCE}
endef

$(eval $(generic-package))
