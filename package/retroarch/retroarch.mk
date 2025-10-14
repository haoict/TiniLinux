################################################################################
#
# Retroarch
#
################################################################################

RETROARCH_VERSION = 1.21.0
RETROARCH_SITE = https://github.com/libretro/RetroArch/releases/download/v${RETROARCH_VERSION}
RETROARCH_SOURCE = retroarch-sourceonly-${RETROARCH_VERSION}.tar.xz
RETROARCH_SOURCE_SUBDIR = retroarch-$(RETROARCH_VERSION)
RETROARCH_DEPENDENCIES = host-pkgconf sdl2 alsa-lib freetype zlib libpng libdrm
RETROARCH_INSTALL_TARGET = YES
RETROARCH_CONF_OPTS = --prefix=$(TARGET_DIR)/usr/local \
    --disable-qt \
    --enable-alsa \
    --disable-pipewire \
    --disable-pulse \
    --enable-udev \
    --disable-opengl1 \
    --disable-x11 \
    --enable-zlib \
    --enable-freetype \
    --disable-discord \
    --disable-vg \
    --disable-sdl \
    --enable-sdl2 \
    --enable-kms \
    --disable-ffmpeg \
    --disable-neon \
    --disable-wayland \
    --enable-opengles --enable-opengles3 --enable-opengles3_1 --disable-opengl \
    --disable-vulkan

# $(eval $(autotools-package))


# Ref: https://github.com/retroroot-linux/retroroot/blob/master/retroroot/package/retroarch/retroarch.mk#L226
define RETROARCH_CONFIGURE_CMDS
	cd $(@D) && \
	PKG_CONF_PATH=pkg-config \
	PKG_CONFIG_PATH="$(HOST_PKG_CONFIG_PATH)" \
	$(TARGET_CONFIGURE_OPTS) \
	$(TARGET_CONFIGURE_ARGS) \
	CROSS_COMPILE="$(TARGET_CROSS)" \
	./configure $(RETROARCH_CONF_OPTS)
endef

define RETROARCH_BUILD_CMDS
	$(TARGET_MAKE_ENV) $(TARGET_CONFIGURE_ARGS) $(MAKE) -C $(@D)
	$(TARGET_MAKE_ENV) $(MAKE) compiler=$(TARGET_CC) -C $(@D)/libretro-common/audio/dsp_filters
	$(TARGET_MAKE_ENV) $(MAKE) compiler=$(TARGET_CC) -C $(@D)/gfx/video_filters
endef

define RETROARCH_INSTALL_TARGET_CMDS
	$(TARGET_MAKE_ENV) $(MAKE) PREFIX=$(TARGET_DIR)/usr/local -C $(@D) install
	$(TARGET_MAKE_ENV) $(MAKE) PREFIX=$(TARGET_DIR)/usr/local -C $(@D)/libretro-common/audio/dsp_filters install
	$(TARGET_MAKE_ENV) $(MAKE) PREFIX=$(TARGET_DIR)/usr/local -C $(@D)/gfx/video_filters install
endef

$(eval $(generic-package))
