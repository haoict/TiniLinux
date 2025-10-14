################################################################################
#
# simple-launcher package
#
################################################################################

SIMPLE_LAUNCHER_VERSION = 1.0
SIMPLE_LAUNCHER_SITE = package/simple-launcher
SIMPLE_LAUNCHER_SITE_METHOD = local

# $(info BR2_DEFCONFIG: $(BR2_DEFCONFIG))
ifeq ($(findstring h700,$(BR2_DEFCONFIG)),h700)
	SIMPLE_LAUNCHER_MAKE_OPTS = UNION_PLATFORM=buildroot_h700
else ifeq ($(findstring rgb30,$(BR2_DEFCONFIG)),rgb30)
	SIMPLE_LAUNCHER_MAKE_OPTS = UNION_PLATFORM=buildroot_rgb30
endif
# $(info SIMPLE_LAUNCHER_MAKE_OPTS: $(SIMPLE_LAUNCHER_MAKE_OPTS))

define SIMPLE_LAUNCHER_BUILD_CMDS
    $(MAKE) $(SIMPLE_LAUNCHER_MAKE_OPTS) CC="$(TARGET_CC)" LD="$(TARGET_LD)" -C $(@D)
endef

define SIMPLE_LAUNCHER_INSTALL_TARGET_CMDS
    mkdir -p $(TARGET_DIR)/usr/local/bin
    $(INSTALL) -D -m 0755 $(@D)/simple-launcher  $(TARGET_DIR)/usr/local/bin
    $(INSTALL) -D -m 0755 $(@D)/simple-launcher-commands.txt  $(TARGET_DIR)/usr/local/bin
    $(INSTALL) -D -m 0755 $(@D)/simple-launcher.ttf  $(TARGET_DIR)/usr/local/bin
endef

$(eval $(generic-package))