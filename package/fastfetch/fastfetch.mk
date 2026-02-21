################################################################################
#
# fastfetch
#
################################################################################

FASTFETCH_VERSION = 2.59.0
FASTFETCH_SITE = https://github.com/fastfetch-cli/fastfetch/releases/download/${FASTFETCH_VERSION}

ifeq ($(findstring x86_64,$(BR2_DEFCONFIG)),x86_64)
	FASTFETCH_SOURCE = fastfetch-linux-amd64.tar.gz
else
	FASTFETCH_SOURCE = fastfetch-linux-aarch64.tar.gz
endif

FASTFETCH_INSTALL_TARGET = YES

define FASTFETCH_INSTALL_TARGET_CMDS
	mkdir -p $(TARGET_DIR)/usr/local/bin
	cp $(@D)/usr/bin/fastfetch $(TARGET_DIR)/usr/local/bin/fastfetch
	chmod +x $(TARGET_DIR)/usr/local/bin/fastfetch
endef

$(eval $(generic-package))
