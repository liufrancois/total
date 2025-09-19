PROJECT := Total

ifeq ($(wildcard /opt/homebrew/opt/llvm/bin/clang++),)
CXX := $(shell which clang++ || which g++)
ifeq ($(CXX),)
$(error No suitable C++ compiler found (clang++ or g++ required))
endif
SUPPORTS_OPENMP := 0
$(warning Homebrew clang++ not found, using $(CXX). OpenMP may not work)
else
CXX := /opt/homebrew/opt/llvm/bin/clang++
SUPPORTS_OPENMP := 1
endif

METAL_CPP_DIR := ../metal-cpp
HAS_XCRUN := $(shell which xcrun >/dev/null 2>&1 && echo 1 || echo 0)

ifeq ($(wildcard $(METAL_CPP_DIR)),)
HAS_METAL_CPP := 0
$(warning metal-cpp not found in $(METAL_CPP_DIR))
else
HAS_METAL_CPP := 1
CXXFLAGS += -I$(METAL_CPP_DIR)
endif

CFITSIO_CFLAGS := $(shell pkg-config --cflags cfitsio 2>/dev/null)
CFITSIO_LIBS   := $(shell pkg-config --libs cfitsio 2>/dev/null)

ifneq ($(CFITSIO_CFLAGS),)
  CXXFLAGS += $(CFITSIO_CFLAGS)
  LDFLAGS  += $(CFITSIO_LIBS)
  HAS_CFITSIO := 1
else
  ifneq ($(wildcard /usr/include/fitsio.h),)
    CXXFLAGS += -I/usr/include
    LDFLAGS  += -lcfitsio
    HAS_CFITSIO := 1
  else
    HAS_CFITSIO := 0
    $(warning cfitsio not found (pkg-config or headers missing))
  endif
endif

CXXFLAGS += -std=c++17 -O3
ifeq ($(SUPPORTS_OPENMP),1)
CXXFLAGS += -fopenmp
LDFLAGS  += -L/opt/homebrew/opt/libomp/lib -lomp
endif

SRC_GPU    := main.cpp GPU.cpp
SRC_CPU    := totalCPU.cpp

METAL_SRC  := ops.metal
AIR        := ops.air
METALLIB   := ops.metallib

TARGET_GPU := $(PROJECT)GPU
TARGET_CPU := $(PROJECT)CPU

all: $(TARGET_CPU) maybeGPU
	@echo
	@if [ -f $(TARGET_CPU) ]; then \
		echo "CPU version compiled successfully ($(TARGET_CPU))"; \
	else \
		echo "CPU version not compiled (cfitsio missing or error)"; \
	fi
	@if [ -f $(TARGET_GPU) ]; then \
		echo "GPU version compiled successfully ($(TARGET_GPU))"; \
	else \
		echo "GPU version not compiled (missing Metal SDK, xcrun, or metal-cpp)"; \
	fi

maybeGPU:
ifeq ($(HAS_XCRUN)$(HAS_METAL_CPP),11)
	$(MAKE) $(TARGET_GPU)
else
	@echo "Skipping GPU compilation: requirements missing (xcrun=$(HAS_XCRUN), metal-cpp=$(HAS_METAL_CPP))"
endif

$(AIR): $(METAL_SRC)
	@echo "Compiling Metal source..."
	xcrun -sdk macosx metal -c $(METAL_SRC) -o $(AIR)

$(METALLIB): $(AIR)
	@echo "Creating Metal library..."
	xcrun -sdk macosx metallib $(AIR) -o $(METALLIB)

$(TARGET_GPU): $(SRC_GPU) $(METALLIB)
	@echo "Compiling GPU version with $(CXX)..."
	$(CXX) $(CXXFLAGS) $(SRC_GPU) $(LDFLAGS) -framework Metal -framework MetalKit -framework Foundation -o $(TARGET_GPU)

$(TARGET_CPU): $(SRC_CPU)
ifeq ($(HAS_CFITSIO),1)
	@echo "Compiling CPU version with $(CXX)..."
	$(CXX) $(CXXFLAGS) $(SRC_CPU) $(LDFLAGS) -o $(TARGET_CPU)
else
	@echo "Skipping CPU compilation: cfitsio not found"
endif

clean:
	@echo "Cleaning build files..."
	rm -f $(AIR) $(METALLIB) $(TARGET_GPU) $(TARGET_CPU)