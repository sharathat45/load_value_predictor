# Load Value Predictor

This project uses the default config O3CPU with the ARM ISA.

## Makefile Commands

- `make build`: Builds the CPU model.
- `make`: Builds the executable file from the Cpp file with necessary CFLAGS.
- `make run`: Runs the executable with the specified CPU Model (default is O3CPU) and caches enabled.
- `make hello`: Runs the `hello` executable on the CPU model with caches enabled.
- `make {BENCHMARK_BIN}`: Runs the benchmark binary specified by "BENCHMARK_BIN = [sjeng lbm milc astar leslie3d namd]" on the CPU model with caches enabled.
   ex: make sjeng
- `make patch`: Creates a patch from (including) "COMMIT_BEGIN" to "COMMIT_END" and stores it in the `scripts/patches` folder.
- `make debug`: builds and displays the ins flow in pipeline for the prev exec/bin run. (for make hello, make run, make sjeng,..)


## Scripts Directory

Any new scripts, benchmarks, Cpp, binary files added to the existing gem5 will be kept in this folder.

- `bin/`: Contains executables or binary files.
- `patches/`: Stores any created patches.
- `results/`: Contains copies of `stats.txt` when running the automation script.
- `benchmark_sweep.sh`: Sweeps through the benchmark and copies the stats to `results` and extracts the IPC from individual stats file.

## Gem5

This is the gem5 simulator.

The main website can be found at http://www.gem5.org

A good starting point is http://www.gem5.org/about, and for
more information about building the simulator and getting started
please see http://www.gem5.org/documentation and
http://www.gem5.org/documentation/learning_gem5/introduction.

To build gem5, you will need the following software: g++ or clang,
Python (gem5 links in the Python interpreter), SCons, zlib, m4, and lastly
protobuf if you want trace capture and playback support. Please see
http://www.gem5.org/documentation/general_docs/building for more details
concerning the minimum versions of these tools.

Once you have all dependencies resolved, type 'scons
build/<CONFIG>/gem5.opt' where CONFIG is one of the options in build_opts like
ARM, NULL, MIPS, POWER, SPARC, X86, Garnet_standalone, etc. This will build an
optimized version of the gem5 binary (gem5.opt) with the the specified
configuration. See http://www.gem5.org/documentation/general_docs/building for
more details and options.

The main source tree includes these subdirectories:
   - build_opts: pre-made default configurations for gem5
   - build_tools: tools used internally by gem5's build process.
   - configs: example simulation configuration scripts
   - ext: less-common external packages needed to build gem5
   - include: include files for use in other programs
   - site_scons: modular components of the build system
   - src: source code of the gem5 simulator
   - system: source for some optional system software for simulated systems
   - tests: regression tests
   - util: useful utility programs and files

To run full-system simulations, you may need compiled system firmware, kernel
binaries and one or more disk images, depending on gem5's configuration and
what type of workload you're trying to run. Many of those resources can be
downloaded from http://resources.gem5.org, and/or from the git repository here:
https://gem5.googlesource.com/public/gem5-resources/

If you have questions, please send mail to gem5-users@gem5.org

Enjoy using gem5 and please share your modifications and extensions.
