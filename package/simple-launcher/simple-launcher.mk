################################################################################
#
# simple-launcher package
#
################################################################################

SIMPLE_LAUNCHER_VERSION = 1.0
SIMPLE_LAUNCHER_SITE = $(BR2_EXTERNAL_TiniLinux_PATH)/package/simple-launcher
SIMPLE_LAUNCHER_SITE_METHOD = local
SIMPLE_LAUNCHER_DEPENDENCIES = sdl2 sdl2_ttf

# $(info BR2_DEFCONFIG: $(BR2_DEFCONFIG))
ifeq ($(findstring h700,$(BR2_DEFCONFIG)),h700)
	SIMPLE_LAUNCHER_MAKE_OPTS = PLATFORM=h700
else ifeq ($(findstring rgb30,$(BR2_DEFCONFIG)),rgb30)
	SIMPLE_LAUNCHER_MAKE_OPTS = PLATFORM=rgb30
else ifeq ($(findstring raspberrypi,$(BR2_DEFCONFIG)),raspberrypi)
	SIMPLE_LAUNCHER_MAKE_OPTS = PLATFORM=raspberrypi
endif
# $(info SIMPLE_LAUNCHER_MAKE_OPTS: $(SIMPLE_LAUNCHER_MAKE_OPTS))

define SIMPLE_LAUNCHER_BUILD_CMDS
    $(MAKE) $(SIMPLE_LAUNCHER_MAKE_OPTS) CC="$(TARGET_CC)" LD="$(TARGET_LD)" -C $(@D)
endef

define SIMPLE_LAUNCHER_INSTALL_TARGET_CMDS
    mkdir -p $(TARGET_DIR)/usr/local/bin
    mkdir -p $(TARGET_DIR)/usr/share/fonts
    $(INSTALL) -D -m 0755 $(@D)/simple-launcher  $(TARGET_DIR)/usr/local/bin
    $(INSTALL) -D -m 0644 $(@D)/simple-launcher-commands.txt  $(TARGET_DIR)/root/
    $(INSTALL) -D -m 0644 $(@D)/Fiery_Turk.ttf  $(TARGET_DIR)/usr/share/fonts/
endef

$(eval $(generic-package))