include makefiles/config.mk

ifeq ($(UNION_PLATFORM),rg35xxplus)
include makefiles/config-rg35xxplus.mk
else ifeq ($(UNION_PLATFORM),r36s)
include makefiles/config-r36s.mk
else ifeq ($(UNION_PLATFORM),trimuisp)
include makefiles/config-trimuisp.mk
else ifeq ($(UNION_PLATFORM),buildroot_rgb30)
include makefiles/config-buildroot-rgb30.mk
else ifeq ($(UNION_PLATFORM),buildroot_h700)
include makefiles/config-buildroot-h700.mk
else ifeq ($(UNION_PLATFORM),buildroot_raspberrypi)
include makefiles/config-buildroot-raspberrypi.mk
else ifeq ($(UNION_PLATFORM),x64)
include makefiles/config-x64.mk
endif

.PHONY: clean
.PHONY: simple-launcher

simple-launcher: simple-launcher.c
	$(CC) -o '$@' '$<' ${CFLAGS} ${LDFLAGS}

check-available-videodriver: check-available-videodriver.c
	$(CC) -o '$@' '$<' ${CFLAGS} ${LDFLAGS}

clean:
	-rm simple-launcher check-available-videodriver