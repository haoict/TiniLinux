VERSION = 2.0.0
PREFIX = /usr
# compiler and linker
CROSS_COMPILE = 
CC = ${CROSS_COMPILE}gcc
SYSROOT = $(shell ${CC} --print-sysroot)
# flags
CFLAGS = -Os -Wall -I. -I${SYSROOT}/usr/include/SDL2 -DVERSION=\"${VERSION}\" -D_GNU_SOURCE=1 -D_REENTRANT  -fPIC -std=gnu11 -flto
LDFLAGS = -lc -L${SYSROOT}/usr/lib -lSDL2 -lpthread -Wl,-Bstatic,-lutil,-Bdynamic -s


ifeq ($(PLATFORM),buildroot_rgb30)
CFLAGS += -DBR2 -DRGB30
else ifeq ($(PLATFORM),buildroot_h700)
CFLAGS += -DBR2 -DH700
else ifeq ($(PLATFORM),buildroot_raspberrypi)
CFLAGS += -DBR2 -DRPI
endif

SRC = $(wildcard src/*.c)

all: options build

options:
	@echo st build options:
	@echo "PLATFORM       = ${PLATFORM}"
	@echo "PREFIX         = ${PREFIX}"
	@echo "CROSS_COMPILE  = ${CROSS_COMPILE}"
	@echo "SYSROOT        = ${SYSROOT}"
	@echo "CFLAGS         = ${CFLAGS}"
	@echo "LDFLAGS        = ${LDFLAGS}"
	@echo "CC             = ${CC}"
	@echo "SRC            = ${SRC}"
	@echo "VERSION        = ${VERSION}"


build:
	${CC} -o simple-terminal ${SRC} ${CFLAGS} ${LDFLAGS}

clean:
	@echo cleaning
	rm -f simple-terminal

.PHONY: all options clean
