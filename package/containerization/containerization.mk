################################################################################
#
# docker
#
################################################################################

CONTAINERIZATION_VERSION = 1.0.0
CONTAINERIZATION_SITE = $(BR2_EXTERNAL_TiniLinux_PATH)/package/containerization
CONTAINERIZATION_SITE_METHOD = local

define CONTAINERIZATION_BUILD_CMDS
	cd $(@D); \
	wget https://download.docker.com/linux/static/stable/aarch64/docker-29.3.0.tgz; \
	tar xzvf docker-29.3.0.tgz; \
	wget https://github.com/docker/compose/releases/download/v5.1.0/docker-compose-linux-aarch64;
endef

define CONTAINERIZATION_INSTALL_TARGET_CMDS
	$(INSTALL) -D -m 0755 $(@D)/docker/*  $(TARGET_DIR)/usr/bin/
	mkdir -p $(TARGET_DIR)/usr/lib/systemd/system/
	mkdir -p ${TARGET_DIR}/etc/systemd/system/multi-user.target.wants/
	$(INSTALL) -D -m 0644 $(@D)/dockerd.service  $(TARGET_DIR)/usr/lib/systemd/system
	ln -sf /usr/lib/systemd/system/dockerd.service ${TARGET_DIR}/etc/systemd/system/multi-user.target.wants/dockerd.service

	mkdir -p ${TARGET_DIR}/usr/local/lib/docker/cli-plugins/
	$(INSTALL) -D -m 0755 $(@D)/docker-compose-linux-aarch64 ${TARGET_DIR}/usr/local/lib/docker/cli-plugins/docker-compose
endef

$(eval $(generic-package))
