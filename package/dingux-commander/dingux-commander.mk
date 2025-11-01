################################################################################
#
# dingux-commander
#
################################################################################

DINGUX_COMMANDER_VERSION = 20251101
DINGUX_COMMANDER_SITE = https://github.com/haoict/dingux-commander.git
DINGUX_COMMANDER_SITE_METHOD = git
DINGUX_COMMANDER_DEPENDENCIES = host-pkgconf sdl2 sdl2_ttf sdl2_image
DINGUX_COMMANDER_INSTALL_TARGET = YES
DINGUX_COMMANDER_CONF_OPTS += "-DCMAKE_BUILD_TYPE=Release \
                                -DWITH_SYSTEM_SDL_TTF=ON \
                                -DWITH_SYSTEM_SDL_GFX=ON \
                                -DCMDR_GAMEPAD_OPEN=ControllerButton::A \
                                -DCMDR_GAMEPAD_PARENT=ControllerButton::B"

define DINGUX_COMMANDER_INSTALL_TARGET_CMDS
    mkdir -p $(TARGET_DIR)/usr/local/bin
    mkdir -p ${TARGET_DIR}/usr/share/commander/res
    $(INSTALL) -D -m 0755 $(@D)/commander  $(TARGET_DIR)/usr/local/bin
    #$(INSTALL) -D -m 0644 $(@D)/res/* $(TARGET_DIR)/usr/share/commander/res
    cp -r $(BR2_EXTERNAL_TiniLinux_PATH)/package/dingux-commander/res $(TARGET_DIR)/usr/share/commander/
endef

$(eval $(cmake-package))
