################################################################################
#
# alterm package
#
################################################################################

ALTERM_VERSION = 1.0
ALTERM_SITE = $(BR2_EXTERNAL_TiniLinux_PATH)/package/alterm
ALTERM_SITE_METHOD = local
ALTERM_DEPENDENCIES = sdl2 sdl2_ttf

ifeq ($(findstring h700,$(BR2_DEFCONFIG)),h700)
	ALTERM_MAKE_OPTS = UNION_PLATFORM=buildroot_h700
else ifeq ($(findstring rgb30,$(BR2_DEFCONFIG)),rgb30)
	ALTERM_MAKE_OPTS = UNION_PLATFORM=buildroot_rgb30
else ifeq ($(findstring pc_x86_64,$(BR2_DEFCONFIG)),pc_x86_64)
	ALTERM_MAKE_OPTS = UNION_PLATFORM=buildroot_pc
else ifeq ($(findstring raspberrypi,$(BR2_DEFCONFIG)),raspberrypi)
	ALTERM_MAKE_OPTS = UNION_PLATFORM=buildroot_rpi
endif

define ALTERM_BUILD_CMDS
	$(MAKE) -C $(@D) $(ALTERM_MAKE_OPTS) CXX="$(TARGET_CXX)" LD="$(TARGET_LD)"
endef

define ALTERM_INSTALL_TARGET_CMDS
	$(INSTALL) -D -m 0755 $(@D)/alterm $(TARGET_DIR)/usr/local/bin/alterm
endef

$(eval $(generic-package))