#
#	Simple Terminal config for RG35XXPLUS SDL12COMPAT
#		based on RG350 ver https://github.com/jamesofarrell/st-sdl
#

INCS += -DRG35XXPLUS_SDL12COMPAT -DSDL12COMPAT

CFLAGS += -mtune=cortex-a53 -mcpu=cortex-a53
