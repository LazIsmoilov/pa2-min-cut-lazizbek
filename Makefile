# Build everything:        make
# Run every test suite:    make test
# Quick benchmark (~1 min): make quick
# Full benchmark:          make bench   (writes results/, used in the report)

CXX      ?= g++
CXXFLAGS := -std=c++23 -O2 -Wall -Wextra
HEADERS  := $(wildcard src/*.hpp)
TESTS    := test_brute_force test_karger test_stoer_wagner test_attack test_generators

.PHONY: all test bench quick clean

all: $(addprefix build/,$(TESTS)) build/bench build/trace_sw

build:
	mkdir -p build

build/test_%: tests/test_%.cpp $(HEADERS) | build
	$(CXX) $(CXXFLAGS) $< -o $@

build/bench: bench/bench.cpp $(HEADERS) | build
	$(CXX) $(CXXFLAGS) $< -o $@

build/trace_sw: scripts/trace_sw.cpp $(HEADERS) | build
	$(CXX) $(CXXFLAGS) $< -o $@

test: $(addprefix build/,$(TESTS))
	@set -e; for t in $(TESTS); do echo "== $$t"; ./build/$$t | tail -1; done

quick: build/bench
	./build/bench --quick

bench: build/bench
	./build/bench
	@{ uname -a; sysctl -n machdep.cpu.brand_string 2>/dev/null || grep -m1 'model name' /proc/cpuinfo; $(CXX) --version | head -1; echo "flags: $(CXXFLAGS)"; } > results/machine.txt

clean:
	rm -rf build
