################################################################################
#
# fake-08 (a pico-8 simulator for retroarch libretro) package
#
################################################################################

FAKE_08_VERSION = f6bab5a7ba521ce440e45d1aeef6122674be6ee9
FAKE_08_SITE = https://github.com/jtothebell/fake-08.git
FAKE_08_SITE_METHOD = git
FAKE_08_GIT_SUBMODULES = YES
FAKE_08_DEPENDENCIES = sdl2

define FAKE_08_BUILD_CMDS
    $(TARGET_MAKE_ENV) $(MAKE) CC="$(TARGET_CC)" CXX="$(TARGET_CXX)" LD="$(TARGET_LD)" -C $(@D)/platform/libretro
endef

define FAKE_08_INSTALL_TARGET_CMDS
    cp -r $(@D)/platform/libretro/fake08_libretro.info $(BR2_EXTERNAL_TiniLinux_PATH)/board/common/ROMS/.config/retroarch/cores/
	cp -r $(@D)/platform/libretro/fake08_libretro.so $(BR2_EXTERNAL_TiniLinux_PATH)/board/common/ROMS/.config/retroarch/cores/
endef

$(eval $(generic-package))