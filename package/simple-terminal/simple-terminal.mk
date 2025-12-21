################################################################################
#
# simple-terminal package
#
################################################################################

SIMPLE_TERMINAL_VERSION = bc7ea8e
SIMPLE_TERMINAL_SITE = https://github.com/haoict/SimpleTerminal.git
SIMPLE_TERMINAL_SITE_METHOD = git
SIMPLE_TERMINAL_DEPENDENCIES = sdl2 sdl2_ttf

# $(info BR2_DEFCONFIG: $(BR2_DEFCONFIG))
SIMPLE_TERMINAL_MAKE_OPTS = VERSION=$(SIMPLE_TERMINAL_VERSION)
ifeq ($(findstring h700,$(BR2_DEFCONFIG)),h700)
	SIMPLE_TERMINAL_MAKE_OPTS += PLATFORM=h700
else ifeq ($(findstring rgb30,$(BR2_DEFCONFIG)),rgb30)
	SIMPLE_TERMINAL_MAKE_OPTS += PLATFORM=rgb30
else ifeq ($(findstring pi,$(BR2_DEFCONFIG)),pi)
	SIMPLE_TERMINAL_MAKE_OPTS += PLATFORM=pi
endif

define SIMPLE_TERMINAL_BUILD_CMDS
    $(MAKE) $(SIMPLE_TERMINAL_MAKE_OPTS) CC="$(TARGET_CC)" LD="$(TARGET_LD)" -C $(@D)
endef

define SIMPLE_TERMINAL_INSTALL_TARGET_CMDS
    mkdir -p $(TARGET_DIR)/usr/local/bin
    $(INSTALL) -D -m 0755 $(@D)/simple-terminal  $(TARGET_DIR)/usr/local/bin
endef

$(eval $(generic-package))