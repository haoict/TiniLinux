# st - simple terminal
# See LICENSE file for copyright and license details.

include makefiles/config.mk

ifeq ($(UNION_PLATFORM),rg35xxplus)
include makefiles/config-rg35xxplus.mk
else ifeq ($(UNION_PLATFORM),rg35xxplus-sdl12compat)
include makefiles/config-rg35xxplus-sdl12compat.mk
else ifeq ($(UNION_PLATFORM),r36s)
include makefiles/config-r36s.mk
else ifeq ($(UNION_PLATFORM),upscale)
include makefiles/config-generic-linux-upscale.mk
else ifeq ($(UNION_PLATFORM),r36s-sdl12compat)
include makefiles/config-r36s-sdl12compat.mk
else ifeq ($(UNION_PLATFORM),rgb30-sdl12compat)
include makefiles/config-rgb30-sdl12compat.mk
endif

SRC = $(wildcard src/*.c)

all: options build

options:
	@echo st build options:
	@echo "UNION_PLATFORM = ${UNION_PLATFORM}"
	@echo "PREFIX         = ${PREFIX}"
	@echo "CROSS_COMPILE  = ${CROSS_COMPILE}"
	@echo "SYSROOT        = ${SYSROOT}"
	@echo "CFLAGS   = ${CFLAGS}"
	@echo "LDFLAGS  = ${LDFLAGS}"
	@echo "CC       = ${CC}"
	@echo "SRC      = ${SRC}"
	@echo "VERSION  = ${VERSION}"


build:
	mkdir -p build
	${CC} -o build/SimpleTerminal ${SRC} ${CFLAGS} ${LDFLAGS}

clean:
	@echo cleaning
	rm -rf build

.PHONY: all options clean
