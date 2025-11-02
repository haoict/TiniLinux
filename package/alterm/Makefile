
TARGET = alterm
SRCDIR = src
BUILDDIR = build

# Source files
SOURCES = $(wildcard $(SRCDIR)/*.cpp)
OBJECTS = $(SOURCES:$(SRCDIR)/%.cpp=$(BUILDDIR)/%.o)

# Default compiler and flags
CXX ?= g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2 -g -Iinclude
LIBS = -lSDL2 -lSDL2_ttf -lpthread

# Platform-specific settings
ifeq ($(UNION_PLATFORM),buildroot_h700)
	CXXFLAGS += -DH700
else ifeq ($(UNION_PLATFORM),buildroot_rgb30)
	CXXFLAGS += -DRGB30
else ifeq ($(UNION_PLATFORM),buildroot_pc)
	CXXFLAGS += -DPC
else ifeq ($(UNION_PLATFORM),buildroot_rpi)
	CXXFLAGS += -DRPI
endif


.PHONY: all clean options

all: options $(TARGET)

options:
	@echo alterm build options:
	@echo "UNION_PLATFORM = ${UNION_PLATFORM}"
	@echo "CXX            = ${CXX}"
	@echo "CXXFLAGS       = ${CXXFLAGS}"
	@echo "LIBS           = ${LIBS}"

$(BUILDDIR):
	mkdir -p $(BUILDDIR)

$(TARGET): $(OBJECTS) | $(BUILDDIR)
	$(CXX) $(OBJECTS) -o $@ $(LIBS)

$(BUILDDIR)/%.o: $(SRCDIR)/%.cpp | $(BUILDDIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	@echo cleaning
	rm -rf $(BUILDDIR)

rebuild: clean all