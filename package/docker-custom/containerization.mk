################################################################################
#
# docker-custom
#
################################################################################

DOCKER_CUSTOM_VERSION = 1.0.0
DOCKER_CUSTOM_SITE = $(BR2_EXTERNAL_TiniLinux_PATH)/package/docker-custom
DOCKER_CUSTOM_SITE_METHOD = local

define DOCKER_CUSTOM_BUILD_CMDS
	cd $(@D); \
	wget https://download.docker.com/linux/static/stable/aarch64/docker-29.3.0.tgz; \
	tar xzvf docker-29.3.0.tgz; \
	wget https://github.com/docker/compose/releases/download/v5.1.0/docker-compose-linux-aarch64;
endef

define DOCKER_CUSTOM_INSTALL_TARGET_CMDS
	$(INSTALL) -D -m 0755 $(@D)/docker/*  $(TARGET_DIR)/usr/bin/
	mkdir -p $(TARGET_DIR)/usr/lib/systemd/system/
	$(INSTALL) -D -m 0644 $(@D)/docker.service $(TARGET_DIR)/usr/lib/systemd/system

	mkdir -p ${TARGET_DIR}/usr/local/lib/docker/cli-plugins/
	$(INSTALL) -D -m 0755 $(@D)/docker-compose-linux-aarch64 ${TARGET_DIR}/usr/local/lib/docker/cli-plugins/docker-compose
endef

$(eval $(generic-package))
