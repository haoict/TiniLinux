################################################################################
#
# rocknix-joypad
#
################################################################################

ROCKNIX_JOYPAD_VERSION = 43245fbce81f28f56e9174e5a8cd94c72c97b161
ROCKNIX_JOYPAD_SITE = https://github.com/ROCKNIX/rocknix-joypad
ROCKNIX_JOYPAD_SITE_METHOD = git
ROCKNIX_JOYPAD_LICENSE = GPL

$(info BR2_DEFCONFIG: $(BR2_DEFCONFIG))
ifeq ($(findstring h700,$(BR2_DEFCONFIG)),h700)
	ROCKNIX_JOYPAD_MODULE_MAKE_OPTS = DEVICE=H700
else ifeq ($(findstring rgb30,$(BR2_DEFCONFIG)),rgb30)
	ROCKNIX_JOYPAD_MODULE_MAKE_OPTS = DEVICE=RK3566
endif
$(info ROCKNIX_JOYPAD_MODULE_MAKE_OPTS: $(ROCKNIX_JOYPAD_MODULE_MAKE_OPTS))

$(eval $(kernel-module))
$(eval $(generic-package))
