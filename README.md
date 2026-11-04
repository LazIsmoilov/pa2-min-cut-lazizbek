# PA2: Minimum Cut — Karger vs Stoer-Wagner

Comparing a randomized algorithm (Karger's contraction) against a deterministic one
(Stoer-Wagner) for global minimum cut on undirected graphs.

Individual project. Primary author of both algorithms: Lazizbek.

## Build & run
### Tests
```
g++ -std=c++23 -O2 -Wall -Wextra tests/test_brute_force.cpp -o build/test_brute_force
./build/test_brute_force
```
```
g++ -std=c++23 -O2 -Wall -Wextra tests/test_karger.cpp -o build/test_karger
./build/test_karger
```
```
g++ -std=c++23 -O2 -Wall -Wextra tests/test_stoer_wagner.cpp -o build/test_sw
./build/test_sw
```

### Stoer-Wagner trace (the source of the hand trace in notes.md)
```
g++ -std=c++23 -O2 -Wall -Wextra scripts/trace_sw.cpp -o build/trace_sw
./build/trace_sw
```

### Attack-prompt regression tests (see ai-logs/attack-karger.md)
```
g++ -std=c++23 -O2 -Wall -Wextra tests/test_attack.cpp -o build/test_attack
./build/test_attack
```

### Generator tests
```
g++ -std=c++23 -O2 -Wall -Wextra tests/test_generators.cpp -o build/test_gen
./build/test_gen
```

## Quick start (Makefile)
```
make          # build all tests, the benchmark and the trace tool
make test     # run every test suite (each should print ALL PASSED)
make quick    # ~1 min smoke benchmark -> results/quick/ (not committed)
make bench    # full benchmark used in the report -> results/timing.csv,
              # results/success.csv, results/machine.txt (several minutes)
```
The benchmark checks every answer before timing it and aborts on any mismatch
(see the comment at the top of bench/bench.cpp for the full method).

## Analysis (tables and figures)
Every number and figure in the report comes from this script, run on the
committed raw data in results/timing.csv and results/success.csv.
```
python3 -m venv .venv
.venv/bin/pip install -r requirements.txt
.venv/bin/python scripts/analyse.py   # -> figures/*.png, results/tables.md, results/key_numbers.json
```

## Video
Unlisted: https://youtu.be/eU8iUQp5UvQ
