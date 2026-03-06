################################################################################
#
# rocknix-joypad
#
################################################################################

ROCKNIX_JOYPAD_VERSION = 7647fdb0fc89cd69b284903bf7707e861df5dc7e
ROCKNIX_JOYPAD_SITE = https://github.com/ROCKNIX/rocknix-joypad.git
ROCKNIX_JOYPAD_SITE_METHOD = git
ROCKNIX_JOYPAD_LICENSE = GPL

# $(info BR2_DEFCONFIG: $(BR2_DEFCONFIG))
ifeq ($(findstring h700,$(BR2_DEFCONFIG)),h700)
	ROCKNIX_JOYPAD_MODULE_MAKE_OPTS = DEVICE=H700
else ifeq ($(findstring rgb30,$(BR2_DEFCONFIG)),rgb30)
	ROCKNIX_JOYPAD_MODULE_MAKE_OPTS = DEVICE=RK3566
endif
# $(info ROCKNIX_JOYPAD_MODULE_MAKE_OPTS: $(ROCKNIX_JOYPAD_MODULE_MAKE_OPTS))

$(eval $(kernel-module))
$(eval $(generic-package))
