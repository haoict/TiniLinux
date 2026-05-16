TIC_80_VERSION = 9b79d496f0e3afcdf43bc5f97f59834114447b63
TIC_80_SITE = https://github.com/nesbox/TIC-80.git
TIC_80_SITE_METHOD = git
TIC_80_GIT_SUBMODULES = YES
TIC_80_DEPENDENCIES = sdl2

TIC_80_CMAKE_OPTS = \
    -DCMAKE_TOOLCHAIN_FILE=$(HOST_DIR)/share/buildroot/toolchainfile.cmake \
    -DBUILD_PLAYER=OFF \
    -DBUILD_SDL=OFF \
    -DBUILD_TOOLS=OFF \
    -DBUILD_LIBRETRO=ON \
    -DBUILD_STATIC=ON

define TIC_80_CONFIGURE_CMDS
    cd $(@D)/build && $(TARGET_MAKE_ENV) $(BR2_CMAKE) .. $(TIC_80_CMAKE_OPTS)
endef

define TIC_80_BUILD_CMDS
    # Patch miniz to avoid large file support issues with non-64 APIs because normal APIs are already 64-bit.
    sed -i 's/\blstat(/lstat64(/g' $(@D)/vendor/zip/src/zip.c && \
    $(TARGET_MAKE_ENV) CFLAGS="-D_LARGEFILE64_SOURCE" $(MAKE) -C $(@D)/build
endef

define TIC_80_INSTALL_TARGET_CMDS
    cp $(BR2_EXTERNAL_TiniLinux_PATH)/package/tic-80/tic80_libretro.info $(BR2_EXTERNAL_TiniLinux_PATH)/board/common/ROMS/.config/retroarch/cores/
	cp $(@D)/build/bin/tic80_libretro.so $(BR2_EXTERNAL_TiniLinux_PATH)/board/common/ROMS/.config/retroarch/cores/
endef

$(eval $(generic-package))