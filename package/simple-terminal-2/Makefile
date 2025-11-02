# Simple Terminal 2 - SDL2-based terminal emulator
# Built from scratch for modern SDL2

# Default compiler and flags
CC ?= gcc
CFLAGS = -Wall -Wextra -std=c99 -O2 -g
TARGET = build/SimpleTerminal2
SRCDIR = src
BUILDDIR = build

# Source files
SOURCES = $(wildcard $(SRCDIR)/*.c)
OBJECTS = $(SOURCES:$(SRCDIR)/%.c=$(BUILDDIR)/%.o)

# Base libraries
LIBS = -lSDL2 -lSDL2_ttf -lutil -lpthread

# Platform-specific settings
ifeq ($(UNION_PLATFORM),buildroot_h700)
	CFLAGS += -DH700
else ifeq ($(UNION_PLATFORM),buildroot_rgb30)
	CFLAGS += -DRGB30
else ifeq ($(UNION_PLATFORM),buildroot_pc)
	CFLAGS += -DPC
else ifeq ($(UNION_PLATFORM),buildroot_rpi)
	CFLAGS += -DRPI
endif

.PHONY: all clean options

all: options $(TARGET)

options:
	@echo Simple Terminal 2 build options:
	@echo "UNION_PLATFORM = ${UNION_PLATFORM}"
	@echo "CC             = ${CC}"
	@echo "CFLAGS         = ${CFLAGS}"
	@echo "LIBS           = ${LIBS}"

$(BUILDDIR):
	mkdir -p $(BUILDDIR)

$(TARGET): $(OBJECTS) | $(BUILDDIR)
	$(CC) $(OBJECTS) -o $@ $(LIBS)

$(BUILDDIR)/%.o: $(SRCDIR)/%.c | $(BUILDDIR)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	@echo cleaning
	rm -rf $(BUILDDIR)
