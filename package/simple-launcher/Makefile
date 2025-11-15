VERSION ?= 1.0.0
# compiler and linker
CROSS_COMPILE ?= 
CC = ${CROSS_COMPILE}gcc
SYSROOT ?= $(shell ${CC} --print-sysroot)
# flags
CFLAGS = -Wall -I. -I${SYSROOT}/usr/include -DVERSION=\"${VERSION}\"
LDFLAGS = -L${SYSROOT}/usr/lib -lSDL2 -lSDL2_ttf -s

ifeq ($(PLATFORM),rgb30)
CFLAGS += -DBR2 -DRGB30
else ifeq ($(PLATFORM),h700)
CFLAGS += -DBR2 -DH700
else ifeq ($(PLATFORM),pi)
CFLAGS += -DBR2 -DRPI
endif

.PHONY: clean
.PHONY: simple-launcher

simple-launcher: simple-launcher.c
	$(CC) -o '$@' '$<' ${CFLAGS} ${LDFLAGS}

clean:
	-rm simple-launcher
