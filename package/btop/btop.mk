################################################################################
#
# btop
#
################################################################################

BTOP_VERSION = 1.4.6
BTOP_SITE = https://github.com/aristocratos/btop/releases/download/v${BTOP_VERSION}

ifeq ($(findstring x86_64,$(BR2_DEFCONFIG)),x86_64)
	BTOP_SOURCE = btop-i686-unknown-linux-musl.tbz
else
	BTOP_SOURCE = btop-aarch64-unknown-linux-musl.tbz
endif

BTOP_INSTALL_TARGET = YES

define BTOP_INSTALL_TARGET_CMDS
	mkdir -p $(TARGET_DIR)/usr/local/bin
	cp $(@D)/btop/bin/btop $(TARGET_DIR)/usr/local/bin
	chmod +x $(TARGET_DIR)/usr/local/bin
endef

$(eval $(generic-package))
