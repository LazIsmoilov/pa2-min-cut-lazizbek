# PA2: Minimum Cut — Karger vs Stoer-Wagner

Comparing a randomized algorithm (Karger's contraction) against a deterministic one
(Stoer-Wagner) for global minimum cut on undirected graphs.

Individual project. Primary author of both algorithms: Laz.

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
