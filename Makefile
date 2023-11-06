BASE_NAME = daxpy
EXECUTABLE = scripts/bin/$(BASE_NAME)
SRC_C = scripts/$(BASE_NAME).cpp
SRC_H = include
ASSEMBLY_FLAG = -S
CC = /usr/bin/g++ -O2 -std=gnu++11 -I $(SRC_H) -L util/m5/build/x86/out/

MODEL = ./build/ECE565-ARM/gem5.opt 
CONFIG = configs/example/se.py 
CONFIG_FLAGS = --cpu-type=O3CPU --maxinsts=1000000 --l1d_size=64kB --l1i_size=16kB --caches --l2cache
HELLO_EXECUTABLE = tests/test-progs/hello/bin/arm/linux/hello

COMMIT_BEGIN = 1e3d7d0
COMMIT_END =  1e3d7d0

BENCHMARK_BIN = sjeng
BENCHMARK_CONFIG = configs/spec/spec_se.py
# sjeng, leslie3d, lbm, astar, milc, namd

$(EXECUTABLE): $(SRC_C)
	$(CC) $(SRC_C) -o $(EXECUTABLE) -lm5

# ./build/ECE565-ARM/gem5.opt configs/example/se.py --cpu-type=MinorCPU --maxinsts=1000000 --l1d_size=64kB --l1i_size=16kB --caches --l2cache -c part1/daxpy-armv7-binary
run: $(EXECUTABLE)
	$(MODEL) $(CONFIG) $(CONFIG_FLAGS) -c $(EXECUTABLE)

build:
	scons-3 USE_HDF5=0 -j `nproc` $(MODEL)
#  build/x86/out/m5

hello:
	$(MODEL) $(CONFIG) $(CONFIG_FLAGS) -c $(HELLO_EXECUTABLE)

benchmark:
	$(MODEL) $(BENCHMARK_CONFIG) $(CONFIG_FLAGS) -b $(BENCHMARK_BIN) 

clean:
	rm -rf $(EXECUTABLE) build/

patch:
	git format-patch --stdout $(COMMIT_BEGIN)^..$(COMMIT_BEGIN) > ./scripts/patches/`date +%Y%m%d_%H%M%S`.patch

.PHONY: run clean build patch benchmark hello
