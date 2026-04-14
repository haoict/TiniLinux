################################################################################
#
# Tinilinux initramfs, ref: https://www.abhik.xyz/concepts/linux/initramfs-boot-process
# 
# ### making console font psfu
# ```bash
# sudo apt install bdf2psf
# git clone https://github.com/IT-Studio-Rech/bdf-fonts.git
# cd bdf-fonts.git
# export font=tom-thumb.bdf
# bdf2psf --fb ${font}.bdf /usr/share/bdf2psf/standard.equivalents /usr/share/bdf2psf/ascii.set+/usr/share/bdf2psf/linux.set+/usr/share/bdf2psf/useful.set 512 ${font}.psfu
# ```
#
################################################################################

INITRAMFS_VERSION = 1.0.0
INITRAMFS_SITE = https://mirrors.slackware.com/slackware/slackware64-current/source/a/mkinitrd
INITRAMFS_SOURCE = busybox-1.37.0.tar.bz2

INITRAMFS_MAKE_ENV = \
	$(TARGET_MAKE_ENV) \
	CFLAGS="$(TARGET_CFLAGS)"

INITRAMFS_MAKE_OPTS = \
	AR="$(TARGET_AR)" \
	NM="$(TARGET_NM)" \
	RANLIB="$(TARGET_RANLIB)" \
	CC="$(TARGET_CC)" \
	ARCH=$(NORMALIZED_ARCH) \
	EXTRA_LDFLAGS="$(TARGET_LDFLAGS)" \
	CROSS_COMPILE="$(TARGET_CROSS)" \


ifeq ($(BR2_PACKAGE_INITRAMFS_NFS),y)
INITRAMFS_KCONFIG_FILE = $(BR2_EXTERNAL_TiniLinux_PATH)/package/initramfs/busybox-nfs.conf
else
INITRAMFS_KCONFIG_FILE = $(BR2_EXTERNAL_TiniLinux_PATH)/package/initramfs/busybox.conf
endif

INITRAMFS_KCONFIG_EDITORS = menuconfig
INITRAMFS_KCONFIG_OPTS = $(INITRAMFS_MAKE_OPTS) HOSTCC="$(HOSTCC_NOCCACHE)"

define INITRAMFS_BUILD_CMDS
	$(INITRAMFS_MAKE_ENV) $(MAKE) $(INITRAMFS_MAKE_OPTS) -C $(@D)
endef

define INITRAMFS_INSTALL_TARGET_CMDS
	# run busybox make install
	rm -rf $(@D)/initramfs
	$(INITRAMFS_MAKE_ENV) $(MAKE) $(INITRAMFS_MAKE_OPTS) CONFIG_PREFIX=$(@D)/initramfs -C $(@D) install

	# prepare filesystem
	mkdir -p $(@D)/initramfs/{dev,proc,sys,run,tmp,newroot}

	# copy files
	cp -r $(BR2_EXTERNAL_TiniLinux_PATH)/package/initramfs/rootfs/* $(@D)/initramfs/
	chmod +x $(@D)/initramfs/init

	# Create cpio archive
	cd $(@D)/initramfs && find . | cpio -o -H newc | gzip > $(BINARIES_DIR)/initramfs
endef

$(eval $(kconfig-package))