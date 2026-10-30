// Stoer-Wagner correctness tests.
//
// 1. Hand-checked graphs, against brute force.
// 2. 300 random graphs, n = 2..14, against brute force.
// 3. 40 larger random graphs, n = 20..40 (too big for brute force), against
//    Karger run at 2x the theoretical trial count. Two completely different
//    algorithms agreeing is strong evidence both are right.
//
// Every returned partition is re-checked: both sides non-empty, and its
// recounted cut value matches what Stoer-Wagner claimed.
#include <cstdio>
#include <cstdlib>
#include <random>

#include "../src/brute_force.hpp"
#include "../src/karger.hpp"
#include "../src/stoer_wagner.hpp"

static int failures = 0;

static bool resultIsConsistent(const Graph& g, const MinCutResult& r) {
    return isValidPartition(g, r.side) && cutValue(g, r.side) == r.value;
}

static void checkAgainstOracle(const char* name, const Graph& g) {
    MinCutResult s = stoerWagnerMinCut(g);
    MinCutResult b = bruteForceMinCut(g);
    bool ok = s.value == b.value && resultIsConsistent(g, s);
    std::printf("%-40s brute %lld stoer-wagner %lld  %s\n", name, (long long)b.value,
                (long long)s.value, ok ? "OK" : "FAIL");
    if (!ok) ++failures;
}

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
        checkAgainstOracle("two triangles + bridge", g);
    }
    {
        Graph g{8, {}};
        for (int i = 0; i < 8; ++i) g.addEdge(i, (i + 1) % 8);
        checkAgainstOracle("cycle n=8", g);
    }
    {
        Graph g{6, {}};
        for (int i = 0; i < 6; ++i)
            for (int j = i + 1; j < 6; ++j) g.addEdge(i, j);
        checkAgainstOracle("complete K6", g);
    }
    {
        Graph g{5, {}};
        for (int i = 0; i < 4; ++i) g.addEdge(i, i + 1);
        checkAgainstOracle("path n=5", g);
    }
    {
        Graph g{4, {}};
        g.addEdge(0, 1); g.addEdge(2, 3);
        checkAgainstOracle("disconnected", g);
    }
    {
        Graph g{2, {}};
        g.addEdge(0, 1); g.addEdge(0, 1); g.addEdge(0, 1);
        checkAgainstOracle("two vertices, 3 parallel edges", g);
    }

    std::printf("\n== 300 random graphs vs brute force ==\n");
    {
        std::mt19937_64 rng(2026);
        std::uniform_int_distribution<int> nDist(2, 14);
        std::uniform_real_distribution<double> pDist(0.15, 0.9);
        int bad = 0;
        for (int i = 0; i < 300; ++i) {
            Graph g = randomGraph(nDist(rng), pDist(rng), rng);
            MinCutResult s = stoerWagnerMinCut(g);
            MinCutResult b = bruteForceMinCut(g);
            if (s.value != b.value || !resultIsConsistent(g, s)) {
                ++bad;
                std::printf("  mismatch on graph %d (n=%d): brute %lld sw %lld\n", i,
                            g.n, (long long)b.value, (long long)s.value);
            }
        }
        std::printf("300 random graphs: %d mismatches  %s\n", bad, bad ? "FAIL" : "OK");
        if (bad) ++failures;
    }

    std::printf("\n== 40 larger random graphs vs Karger ==\n");
    {
        std::mt19937_64 rng(31);
        std::uniform_int_distribution<int> nDist(20, 40);
        std::uniform_real_distribution<double> pDist(0.1, 0.6);
        int bad = 0;
        for (int i = 0; i < 40; ++i) {
            Graph g = randomGraph(nDist(rng), pDist(rng), rng);
            MinCutResult s = stoerWagnerMinCut(g);
            MinCutResult k = kargerMinCut(g, 2 * kargerTrialsForConfidence(g.n), 5000 + i);
            if (s.value != k.value || !resultIsConsistent(g, s)) {
                ++bad;
                std::printf("  mismatch on graph %d (n=%d): karger %lld sw %lld\n", i,
                            g.n, (long long)k.value, (long long)s.value);
            }
        }
        std::printf("40 larger graphs: %d mismatches  %s\n", bad, bad ? "FAIL" : "OK");
        if (bad) ++failures;
    }

    std::printf("\n%s (%d failure%s)\n", failures ? "FAILED" : "ALL PASSED",
                failures, failures == 1 ? "" : "s");
    return failures ? EXIT_FAILURE : EXIT_SUCCESS;
}
