################################################################################
#
# fbdoom package
#
################################################################################

FBDOOM_VERSION = 1870c619e8cb83fb30bfe57008920b529633ef64
FBDOOM_SITE = https://github.com/haoict/fbDOOM.git
FBDOOM_SITE_METHOD = git

define FBDOOM_BUILD_CMDS
    $(MAKE) CROSS_COMPILE=$(TARGET_CROSS) CC=$(TARGET_CC) LD="$(TARGET_LD)" NOSDL=1 -C $(@D)/fbdoom
endef

define FBDOOM_INSTALL_TARGET_CMDS
    mkdir -p $(TARGET_DIR)/usr/local/bin
    $(INSTALL) -D -m 0755 $(@D)/fbdoom/fbdoom  $(TARGET_DIR)/usr/local/bin

    mkdir -p $(TARGET_DIR)/usr/share/games/doom/
    wget -O- https://github.com/nneonneo/universal-doom/raw/refs/heads/main/DOOM1.WAD > $(TARGET_DIR)/usr/share/games/doom/DOOM1.WAD
endef

$(eval $(generic-package))
