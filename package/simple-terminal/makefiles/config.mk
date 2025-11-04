#
#	Simple Terminal config
#		based on RG350 ver https://github.com/jamesofarrell/st-sdl
#

# st version
#GIT_SHA_FETCH := $(shell git rev-parse HEAD | cut -c 1-7)
#VERSION = "1.0.2-$(shell git rev-parse HEAD | cut -c 1-7)"
VERSION = "1.6.0"

# Customize below to fit your system

# paths
PREFIX = /usr
CROSS_COMPILE = 

# compiler and linker
CC = ${CROSS_COMPILE}gcc
SYSROOT = $(shell ${CC} --print-sysroot)

# includes and libs
INCS = -I. -I${SYSROOT}/usr/include/SDL2
LIBS = -lc -L${SYSROOT}/usr/lib -lSDL2 -lpthread -Wl,-Bstatic,-lutil,-Bdynamic

# flags
CPPFLAGS = -DVERSION=\"${VERSION}\" -D_GNU_SOURCE=1 -D_REENTRANT 
CFLAGS += -Os -Wall ${INCS} ${CPPFLAGS} -fPIC -std=gnu11 -flto -Wno-unused-result -Wno-unused-variable
LDFLAGS += ${LIBS} -s
