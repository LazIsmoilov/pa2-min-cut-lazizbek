// Karger correctness tests, checked against the brute-force oracle.
//
// 1. Hand-checked graphs (same ones as the oracle tests).
// 2. Cycle: every single trial must return 2 (see ai-logs/failures.md #1).
// 3. 300 random graphs, n = 2..14, compared to brute force.
//
// Every returned partition is also re-checked: both sides non-empty, and its
// recounted cut value matches what Karger claimed.
#include <cstdio>
#include <cstdlib>
#include <random>

#include "../src/brute_force.hpp"
#include "../src/karger.hpp"

static int failures = 0;

static bool resultIsConsistent(const Graph& g, const MinCutResult& r) {
    return isValidPartition(g, r.side) && cutValue(g, r.side) == r.value;
}

static void checkAgainstOracle(const char* name, const Graph& g, uint64_t seed) {
    int64_t trials = 2 * kargerTrialsForConfidence(g.n);
    MinCutResult k = kargerMinCut(g, trials, seed);
    MinCutResult b = bruteForceMinCut(g);
    bool ok = k.value == b.value && resultIsConsistent(g, k);
    std::printf("%-40s brute %lld karger %lld  %s\n", name, (long long)b.value,
                (long long)k.value, ok ? "OK" : "FAIL");
    if (!ok) ++failures;
}

// Random multigraph: each pair gets an edge with probability p. Parallel
// edges added with a small extra chance so the multigraph case is covered.
static Graph randomGraph(int n, double p, std::mt19937_64& rng) {
    std::bernoulli_distribution edge(p), extra(0.1);
    Graph g{n, {}};
    for (int i = 0; i < n; ++i)
        for (int j = i + 1; j < n; ++j)
            if (edge(rng)) {
                g.addEdge(i, j);
                if (extra(rng)) g.addEdge(i, j);
            }
    return g;
}

int main() {
    std::printf("== Hand-checked graphs ==\n");
    {
        Graph g{6, {}};
        g.addEdge(0, 1); g.addEdge(1, 2); g.addEdge(0, 2);
        g.addEdge(3, 4); g.addEdge(4, 5); g.addEdge(3, 5);
        g.addEdge(2, 5);
        checkAgainstOracle("two triangles + bridge", g, 1);
    }
    {
        Graph g{6, {}};
        for (int i = 0; i < 6; ++i)
            for (int j = i + 1; j < 6; ++j) g.addEdge(i, j);
        checkAgainstOracle("complete K6", g, 2);
    }
    {
        Graph g{5, {}};
        for (int i = 0; i < 4; ++i) g.addEdge(i, i + 1);
        checkAgainstOracle("path n=5", g, 3);
    }
    {
        Graph g{4, {}};
        g.addEdge(0, 1); g.addEdge(2, 3);
        checkAgainstOracle("disconnected", g, 4);
    }
    {
        Graph g{2, {}};
        g.addEdge(0, 1); g.addEdge(0, 1); g.addEdge(0, 1);
        checkAgainstOracle("two vertices, 3 parallel edges", g, 5);
    }

    std::printf("\n== Cycle: every single trial must give 2 ==\n");
    {
        const int n = 12, trials = 10000;
        Graph g{n, {}};
        for (int i = 0; i < n; ++i) g.addEdge(i, (i + 1) % n);
        std::mt19937_64 rng(6);
        std::vector<size_t> order;
        int bad = 0;
        for (int t = 0; t < trials; ++t) {
            MinCutResult r = kargerTrial(g, rng, order);
            if (r.value != 2 || !resultIsConsistent(g, r)) ++bad;
        }
        std::printf("cycle n=12, %d trials: %d not equal to 2  %s\n", trials, bad,
                    bad ? "FAIL" : "OK");
        if (bad) ++failures;
    }

    std::printf("\n== 300 random graphs vs brute force ==\n");
    {
        std::mt19937_64 rng(2026);
        std::uniform_int_distribution<int> nDist(2, 14);
        std::uniform_real_distribution<double> pDist(0.15, 0.9);
        int bad = 0;
        for (int i = 0; i < 300; ++i) {
            Graph g = randomGraph(nDist(rng), pDist(rng), rng);
            int64_t trials = 2 * kargerTrialsForConfidence(g.n);
            MinCutResult k = kargerMinCut(g, trials, 1000 + i);
            MinCutResult b = bruteForceMinCut(g);
            if (k.value != b.value || !resultIsConsistent(g, k)) {
                ++bad;
                std::printf("  mismatch on graph %d (n=%d): brute %lld karger %lld\n",
                            i, g.n, (long long)b.value, (long long)k.value);
            }
        }
        std::printf("300 random graphs: %d mismatches  %s\n", bad, bad ? "FAIL" : "OK");
        if (bad) ++failures;
    }

    std::printf("\n%s (%d failure%s)\n", failures ? "FAILED" : "ALL PASSED",
                failures, failures == 1 ? "" : "s");
    return failures ? EXIT_FAILURE : EXIT_SUCCESS;
}
