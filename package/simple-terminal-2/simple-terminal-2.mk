################################################################################
#
# simple-terminal-2 package
#
################################################################################

SIMPLE_TERMINAL_2_VERSION = 1.0
SIMPLE_TERMINAL_2_SITE = $(BR2_EXTERNAL_TiniLinux_PATH)/package/simple-terminal-2
SIMPLE_TERMINAL_2_SITE_METHOD = local
SIMPLE_TERMINAL_2_DEPENDENCIES = sdl2 sdl2_ttf

ifeq ($(findstring h700,$(BR2_DEFCONFIG)),h700)
	SIMPLE_TERMINAL_2_MAKE_OPTS = UNION_PLATFORM=buildroot_h700
else ifeq ($(findstring rgb30,$(BR2_DEFCONFIG)),rgb30)
	SIMPLE_TERMINAL_2_MAKE_OPTS = UNION_PLATFORM=buildroot_rgb30
else ifeq ($(findstring pc_x86_64,$(BR2_DEFCONFIG)),pc_x86_64)
	SIMPLE_TERMINAL_2_MAKE_OPTS = UNION_PLATFORM=buildroot_pc
else ifeq ($(findstring raspberrypi,$(BR2_DEFCONFIG)),raspberrypi)
	SIMPLE_TERMINAL_2_MAKE_OPTS = UNION_PLATFORM=buildroot_rpi
endif

define SIMPLE_TERMINAL_2_BUILD_CMDS
	$(MAKE) -C $(@D) $(SIMPLE_TERMINAL_2_MAKE_OPTS) CC="$(TARGET_CC)" LD="$(TARGET_LD)"
endef

define SIMPLE_TERMINAL_2_INSTALL_TARGET_CMDS
	$(INSTALL) -D -m 0755 $(@D)/build/SimpleTerminal2 $(TARGET_DIR)/usr/local/bin/SimpleTerminal2
endef

$(eval $(generic-package))