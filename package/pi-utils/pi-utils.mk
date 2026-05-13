PI_UTILS_VERSION = 061dfd3abd1155aa068738deec8feac3fe7806e1
PI_UTILS_SITE = https://github.com/raspberrypi/utils.git
PI_UTILS_SITE_METHOD = git
PI_UTILS_DEPENDENCIES = host-pkgconf host-dtc dtc
PI_UTILS_INSTALL_TARGET = YES

PI_UTILS_CMAKE_OPTS = \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_TOOLCHAIN_FILE=$(HOST_DIR)/share/buildroot/toolchainfile.cmake \
    -DCMAKE_INSTALL_PREFIX=/usr

define PI_UTILS_CONFIGURE_CMDS
    # Build vcgencmd
	mkdir -p $(@D)/vcgencmd/build && cd $(@D)/vcgencmd/build && $(TARGET_MAKE_ENV) $(BR2_CMAKE) $(PI_UTILS_CMAKE_OPTS) ..
    # Build dtmerge
    mkdir -p $(@D)/dtmerge/build && cd $(@D)/dtmerge/build && $(TARGET_MAKE_ENV) $(BR2_CMAKE) $(PI_UTILS_CMAKE_OPTS) -DCMAKE_C_FLAGS="-I$(HOST_DIR)/include/libfdt" ..
endef

define PI_UTILS_BUILD_CMDS
	$(TARGET_MAKE_ENV) $(MAKE) -C $(@D)/vcgencmd/build
	$(TARGET_MAKE_ENV) $(MAKE) -C $(@D)/dtmerge/build
endef


define PI_UTILS_INSTALL_TARGET_CMDS
    mkdir -p $(TARGET_DIR)/usr/local/bin
    $(INSTALL) -D -m 0755 $(@D)/vcgencmd/build/vcgencmd $(TARGET_DIR)/usr/local/bin
    $(INSTALL) -D -m 0755 $(BR2_EXTERNAL_TiniLinux_PATH)/package/pi-utils/pi-get-throttled.sh $(TARGET_DIR)/usr/local/bin
    $(INSTALL) -D -m 0755 $(@D)/dtmerge/build/dtoverlay $(TARGET_DIR)/usr/local/bin

endef

$(eval $(generic-package))
