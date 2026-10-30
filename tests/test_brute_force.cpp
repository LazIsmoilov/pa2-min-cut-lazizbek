// Sanity tests for the brute-force oracle on graphs where the min cut is
// known by hand. If the oracle is wrong, every later test is meaningless,
// so it gets checked first.
#include <cstdio>
#include <cstdlib>

#include "../src/brute_force.hpp"

static int failures = 0;

static void check(const char* name, const Graph& g, int64_t expected) {
    MinCutResult r = bruteForceMinCut(g);
    bool ok = r.value == expected && isValidPartition(g, r.side) &&
              cutValue(g, r.side) == r.value;
    std::printf("%-40s expected %lld got %lld  %s\n", name, (long long)expected,
                (long long)r.value, ok ? "OK" : "FAIL");
    if (!ok) ++failures;
}

int main() {
    // Two triangles joined by one edge: min cut 1 (the bridge).
    {
        Graph g{6, {}};
        g.addEdge(0, 1); g.addEdge(1, 2); g.addEdge(0, 2);
        g.addEdge(3, 4); g.addEdge(4, 5); g.addEdge(3, 5);
        g.addEdge(2, 5);
        check("two triangles + bridge", g, 1);
    }
    // Cycle of 8: any split into two arcs cuts 2 edges.
    {
        Graph g{8, {}};
        for (int i = 0; i < 8; ++i) g.addEdge(i, (i + 1) % 8);
        check("cycle n=8", g, 2);
    }
    // Complete graph K6: best is one vertex alone, cutting n-1 = 5 edges.
    {
        Graph g{6, {}};
        for (int i = 0; i < 6; ++i)
            for (int j = i + 1; j < 6; ++j) g.addEdge(i, j);
        check("complete K6", g, 5);
    }
    // Path of 5: cut any single edge.
    {
        Graph g{5, {}};
        for (int i = 0; i < 4; ++i) g.addEdge(i, i + 1);
        check("path n=5", g, 1);
    }
    // Disconnected (two separate edges): min cut 0.
    {
        Graph g{4, {}};
        g.addEdge(0, 1); g.addEdge(2, 3);
        check("disconnected", g, 0);
    }
    // Two vertices joined by 3 parallel edges: only one split, cuts 3.
    {
        Graph g{2, {}};
        g.addEdge(0, 1); g.addEdge(0, 1); g.addEdge(0, 1);
        check("two vertices, 3 parallel edges", g, 3);
    }

    std::printf("\n%s (%d failure%s)\n", failures ? "FAILED" : "ALL PASSED",
                failures, failures == 1 ? "" : "s");
    return failures ? EXIT_FAILURE : EXIT_SUCCESS;
}
