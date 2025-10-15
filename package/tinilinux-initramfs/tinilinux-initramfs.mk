################################################################################
#
# Tinilinux initramfs, ref: https://www.abhik.xyz/concepts/linux/initramfs-boot-process
#
################################################################################

TINILINUX_INITRAMFS_VERSION = 1.0.0
TINILINUX_INITRAMFS_SITE = https://www.busybox.net/downloads
TINILINUX_INITRAMFS_SOURCE = busybox-1.37.0.tar.bz2

TINILINUX_INITRAMFS_MAKE_OPTS = \
	AR="$(TARGET_AR)" \
	NM="$(TARGET_NM)" \
	RANLIB="$(TARGET_RANLIB)" \
	CC="$(TARGET_CC)" \
	ARCH=$(NORMALIZED_ARCH) \
	EXTRA_LDFLAGS="$(TARGET_LDFLAGS)" \
	CROSS_COMPILE="$(TARGET_CROSS)" \

define TINILINUX_INITRAMFS_BUILD_CMDS
	# copy custom busybox config from package dir to build dir
	cp $(TOPDIR)/package/tinilinux-initramfs/busybox.conf $(@D)/.config
	$(TARGET_MAKE_ENV) CFLAGS="$(TARGET_CFLAGS)" $(MAKE) $(TINILINUX_INITRAMFS_MAKE_OPTS) -C $(@D)
endef

define TINILINUX_INITRAMFS_INSTALL_TARGET_CMDS
	# run busybox make install
	rm -rf $(@D)/initramfs
	$(TARGET_MAKE_ENV) $(MAKE) $(TINILINUX_INITRAMFS_MAKE_OPTS) CONFIG_PREFIX=$(@D)/initramfs -C $(@D) install

	# prepare filesystem
	mkdir -p $(@D)/initramfs/{dev,proc,sys,run,tmp,newroot}
	cp $(TOPDIR)/package/tinilinux-initramfs/init $(@D)/initramfs
	chmod +x $(@D)/initramfs/init
	
	# Create cpio archive
	cd $(@D)/initramfs && find . | cpio -o -H newc | gzip > $(BINARIES_DIR)/initramfs
endef

$(eval $(generic-package))