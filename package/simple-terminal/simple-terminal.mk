################################################################################
#
# simple-terminal package
#
################################################################################

SIMPLE_TERMINAL_VERSION = 1.0
SIMPLE_TERMINAL_SITE = $(BR2_EXTERNAL_TiniLinux_PATH)/package/simple-terminal
SIMPLE_TERMINAL_SITE_METHOD = local
SIMPLE_TERMINAL_DEPENDENCIES = sdl2

# $(info BR2_DEFCONFIG: $(BR2_DEFCONFIG))
ifeq ($(findstring h700,$(BR2_DEFCONFIG)),h700)
	SIMPLE_TERMINAL_MAKE_OPTS = PLATFORM=buildroot_h700
else ifeq ($(findstring rgb30,$(BR2_DEFCONFIG)),rgb30)
	SIMPLE_TERMINAL_MAKE_OPTS = PLATFORM=buildroot_rgb30
endif

SIMPLE_TERMINAL_MAKE_OPTS = PLATFORM=buildroot_rgb30

define SIMPLE_TERMINAL_BUILD_CMDS
    $(MAKE) $(SIMPLE_TERMINAL_MAKE_OPTS) CC="$(TARGET_CC)" LD="$(TARGET_LD)" -C $(@D)
endef

define SIMPLE_TERMINAL_INSTALL_TARGET_CMDS
    mkdir -p $(TARGET_DIR)/usr/local/bin
    $(INSTALL) -D -m 0755 $(@D)/build/SimpleTerminal  $(TARGET_DIR)/usr/local/bin
endef

$(eval $(generic-package))