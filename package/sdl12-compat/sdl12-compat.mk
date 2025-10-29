################################################################################
#
# sdl12_compat
#
################################################################################

SDL12_COMPAT_VERSION = bec4bd04b5fdc19bc7bf6a7a417ae79978b0344a
SDL12_COMPAT_SITE = https://github.com/libsdl-org/sdl12-compat.git
SDL12_COMPAT_SITE_METHOD = git
SDL12_COMPAT_DEPENDENCIES = host-pkgconf sdl2
#SDL12_COMPAT_CONF_OPTS = -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr/local
SDL12_COMPAT_CONF_OPTS = -DCMAKE_BUILD_TYPE=Release
SDL12_COMPAT_INSTALL_STAGING = YES
SDL12_COMPAT_INSTALL_TARGET = YES

define SDL12_COMPAT_INSTALL_TARGET_CMDS
    $(INSTALL) -D -m 0644 $(@D)/libSDL-1.2.so.0  $(TARGET_DIR)/usr/lib
endef

$(eval $(cmake-package))
