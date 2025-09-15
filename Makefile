CXX := /opt/homebrew/opt/llvm/bin/clang++
CXXFLAGS := -std=c++17 -stdlib=libc++ -O3 -fopenmp -fno-objc-arc \
    -I../metal-cpp \
    -I$(shell brew --prefix cfitsio)/include
LDFLAGS := -L/opt/homebrew/opt/libomp/lib \
    -L$(shell brew --prefix cfitsio)/lib \
    -framework Metal -framework Foundation -framework MetalKit \
    -lcfitsio

SRC := main.cpp GPU.cpp

METAL_SRC := ops.metal
AIR := ops.air
METALLIB := ops.metallib

TARGET := total
TARGET_CPU := totalCPU
SRC_CPU := totalCPU.cpp

all: $(TARGET)

$(AIR): $(METAL_SRC)
	xcrun -sdk macosx metal -c $(METAL_SRC) -o $(AIR)

$(METALLIB): $(AIR)
	xcrun -sdk macosx metallib $(AIR) -o $(METALLIB)

$(TARGET): $(SRC) $(METALLIB)
	$(CXX) $(CXXFLAGS) $(SRC) $(LDFLAGS) -o $(TARGET)

CPU: $(SRC_CPU)
	$(CXX) -I$(shell brew --prefix cfitsio)/include -L$(shell brew --prefix cfitsio)/lib -lcfitsio -O3 -fopenmp $(SRC_CPU) -o $(TARGET_CPU)

clean:
	rm -f $(AIR) $(METALLIB) $(TARGET) $(TARGET_CPU)
