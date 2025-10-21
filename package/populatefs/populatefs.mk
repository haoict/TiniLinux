################################################################################
#
# host populatefs
#
################################################################################

HOST_POPULATEFS_VERSION = a1402683974e69b317bdb64246003b551bd3ff8c
HOST_POPULATEFS_SITE = https://github.com/lipnitsk/populatefs.git
HOST_POPULATEFS_SITE_METHOD = git
HOST_POPULATEFS_DEPENDS_HOST = "host-e2fsprogs"

define HOST_POPULATEFS_BUILD_CMDS
    # $(HOST_MAKE_ENV) ${HOST_CONFIGURE_OPTS} $(MAKE) -C $(@D)
	$(HOST_MAKE_ENV) EXTRA_CFLAGS="-DHAVE_GETOPT_H=1 -I$(HOST_DIR)/include" EXTRA_LDFLAGS="-Wl,-rpath,$(HOST_DIR)/lib -L$(HOST_DIR)/lib" $(MAKE) -C $(@D)
endef

define HOST_POPULATEFS_INSTALL_CMDS
	mkdir -p $(HOST_DIR)/bin
	$(HOST_MAKE_ENV) $(MAKE) -C $(@D) DESTDIR=$(HOST_DIR) install
endef

$(eval $(host-generic-package))
