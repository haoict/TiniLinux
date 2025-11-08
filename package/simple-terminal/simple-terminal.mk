################################################################################
#
# simple-terminal package
#
################################################################################

SIMPLE_TERMINAL_VERSION = 2.0.0
SIMPLE_TERMINAL_SITE = $(BR2_EXTERNAL_TiniLinux_PATH)/package/simple-terminal
SIMPLE_TERMINAL_SITE_METHOD = local
SIMPLE_TERMINAL_DEPENDENCIES = sdl2 sdl2_ttf

# $(info BR2_DEFCONFIG: $(BR2_DEFCONFIG))
SIMPLE_TERMINAL_MAKE_OPTS = VERSION=$(SIMPLE_TERMINAL_VERSION)
ifeq ($(findstring h700,$(BR2_DEFCONFIG)),h700)
	SIMPLE_TERMINAL_MAKE_OPTS += PLATFORM=h700
else ifeq ($(findstring rgb30,$(BR2_DEFCONFIG)),rgb30)
	SIMPLE_TERMINAL_MAKE_OPTS += PLATFORM=rgb30
else ifeq ($(findstring raspberrypi,$(BR2_DEFCONFIG)),raspberrypi)
	SIMPLE_TERMINAL_MAKE_OPTS += PLATFORM=raspberrypi
endif

define SIMPLE_TERMINAL_BUILD_CMDS
    $(MAKE) $(SIMPLE_TERMINAL_MAKE_OPTS) CC="$(TARGET_CC)" LD="$(TARGET_LD)" -C $(@D)
endef

define SIMPLE_TERMINAL_INSTALL_TARGET_CMDS
    mkdir -p $(TARGET_DIR)/usr/local/bin
    $(INSTALL) -D -m 0755 $(@D)/simple-terminal  $(TARGET_DIR)/usr/local/bin
endef

$(eval $(generic-package))