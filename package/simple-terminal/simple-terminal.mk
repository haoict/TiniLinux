################################################################################
#
# simple-terminal package
#
################################################################################

SIMPLE_TERMINAL_VERSION = 1.0
SIMPLE_TERMINAL_SITE = package/simple-terminal
SIMPLE_TERMINAL_SITE_METHOD = local

define SIMPLE_TERMINAL_BUILD_CMDS
    $(MAKE) UNION_PLATFORM=rg35xxplus-sdl12compat CC="$(TARGET_CC)" LD="$(TARGET_LD)" -C $(@D)
endef

define SIMPLE_TERMINAL_INSTALL_TARGET_CMDS
    mkdir -p $(TARGET_DIR)/usr/local/bin
    $(INSTALL) -D -m 0755 $(@D)/build/SimpleTerminal  $(TARGET_DIR)/usr/local/bin
endef

$(eval $(generic-package))