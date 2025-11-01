################################################################################
#
# Tinilinux initramfs, ref: https://www.abhik.xyz/concepts/linux/initramfs-boot-process
#
################################################################################

INITRAMFS_VERSION = 1.0.0
INITRAMFS_SITE = https://www.busybox.net/downloads
INITRAMFS_SOURCE = busybox-1.37.0.tar.bz2

INITRAMFS_MAKE_OPTS = \
	AR="$(TARGET_AR)" \
	NM="$(TARGET_NM)" \
	RANLIB="$(TARGET_RANLIB)" \
	CC="$(TARGET_CC)" \
	ARCH=$(NORMALIZED_ARCH) \
	EXTRA_LDFLAGS="$(TARGET_LDFLAGS)" \
	CROSS_COMPILE="$(TARGET_CROSS)" \

define INITRAMFS_BUILD_CMDS
	# copy custom busybox config from package dir to build dir
	cp $(BR2_EXTERNAL_TiniLinux_PATH)/package/initramfs/busybox.conf $(@D)/.config
	$(TARGET_MAKE_ENV) CFLAGS="$(TARGET_CFLAGS)" $(MAKE) $(INITRAMFS_MAKE_OPTS) -C $(@D)
endef

define INITRAMFS_INSTALL_TARGET_CMDS
	# run busybox make install
	rm -rf $(@D)/initramfs
	$(TARGET_MAKE_ENV) $(MAKE) $(INITRAMFS_MAKE_OPTS) CONFIG_PREFIX=$(@D)/initramfs -C $(@D) install

	# prepare filesystem
	mkdir -p $(@D)/initramfs/{dev,proc,sys,run,tmp,newroot}
	cp $(BR2_EXTERNAL_TiniLinux_PATH)/package/initramfs/init $(@D)/initramfs
	chmod +x $(@D)/initramfs/init
	
	# Create cpio archive
	cd $(@D)/initramfs && find . | cpio -o -H newc | gzip > $(BINARIES_DIR)/initramfs
endef

$(eval $(generic-package))