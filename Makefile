BASE_NAME = daxpy
EXECUTABLE = scripts/bin/$(BASE_NAME)
SRC_C = scripts/$(BASE_NAME).cpp
SRC_H = include
ASSEMBLY_FLAG = -S
CC = /usr/bin/g++ -O2 -std=gnu++11 -I $(SRC_H) -L util/m5/build/x86/out/

MODEL = ./build/ECE565-ARM/gem5.opt 
CONFIG = ./configs/example/se.py 
CONFIG_FLAGS = --cpu-type=O3CPU --maxinsts=1000000 --l1d_size=64kB --l1i_size=16kB --caches --l2cache
HELLO_EXECUTABLE = tests/test-progs/hello/bin/arm/linux/hello

COMMIT_BEGIN = 1e3d7d0
COMMIT_END =  1e3d7d0

BENCHMARK_CONFIG = configs/spec/spec_se.py

$(EXECUTABLE): $(SRC_C)
	$(CC) $(SRC_C) -o $(EXECUTABLE) -lm5

build:
	scons-3 USE_HDF5=0 -j `nproc` $(MODEL)
#  build/x86/out/m5

run: $(EXECUTABLE)
	$(MODEL) $(CONFIG) $(CONFIG_FLAGS) -c $(EXECUTABLE)
	@echo "$(CONFIG) $(CONFIG_FLAGS) -c $(EXECUTABLE)" > last_command

hello:
	$(MODEL) $(CONFIG) $(CONFIG_FLAGS) -c $(HELLO_EXECUTABLE)
	@echo "$(CONFIG) $(CONFIG_FLAGS) -c $(HELLO_EXECUTABLE)" > last_command

BENCHMARKS = sjeng lbm milc astar leslie3d namd
$(BENCHMARKS):
	BENCHMARK_BIN=$@; \
	$(MODEL) $(BENCHMARK_CONFIG) $(CONFIG_FLAGS) -b $$BENCHMARK_BIN
	@echo "$(BENCHMARK_CONFIG) $(CONFIG_FLAGS) -b $@" > last_command

patch:
	git format-patch --stdout $(COMMIT_BEGIN)^..$(COMMIT_END) > ./scripts/patches/`date +%Y%m%d_%H%M%S`.patch

debug: 
	$(MODEL) --debug-flags=O3PipeView --debug-start=1 --debug-file=trace.out `cat last_command` -m 1000000
	./util/o3-pipeview.py -c 500 -o pipeview.out --color m5out/trace.out
	less -r pipeview.out

clean:
	rm -rf $(EXECUTABLE) 
# rm -rf $(EXECUTABLE) build/

.PHONY: run clean build patch benchmark hello debug
