################################################################################
#
# btop
#
################################################################################

BTOP_VERSION = 1.4.5
BTOP_SITE = https://github.com/aristocratos/btop/releases/download/v${BTOP_VERSION}
BTOP_SOURCE = btop-aarch64-linux-musl.tbz
BTOP_INSTALL_TARGET = YES

define BTOP_INSTALL_TARGET_CMDS
	ls -la $(@D)/btop/bin
	$(INSTALL) -D -m 0755 $(@D)/btop/bin/btop  $(TARGET_DIR)/usr/local/bin
endef

$(eval $(generic-package))
