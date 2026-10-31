// Shared graph representation used by every algorithm and the benchmark harness.
// Undirected, unweighted multigraph: parallel edges allowed, self-loops not.
#pragma once
#include <cstdint>
#include <stdexcept>
#include <vector>

struct Edge {
    int u, v;
};

struct Graph {
    int n = 0;
    std::vector<Edge> edges;

    // Rejects out-of-range endpoints and self-loops here, once, so no
    // algorithm ever indexes outside [0, n). Found via the Stoer-Wagner
    // attack prompt (see ai-logs/attack-stoer-wagner.md).
    void addEdge(int u, int v) {
        if (u < 0 || v < 0 || u >= n || v >= n)
            throw std::invalid_argument("edge endpoint outside [0, n)");
        if (u == v) throw std::invalid_argument("self-loops are not allowed");
        edges.push_back({u, v});
    }
};

// side[i] is 0 or 1. Both sides must be non-empty for a valid cut.
struct MinCutResult {
    int64_t value = 0;
    std::vector<char> side;
};

// Counts edges crossing the partition. Used to double-check that the
// partition an algorithm returns really has the value it claims.
inline int64_t cutValue(const Graph& g, const std::vector<char>& side) {
    int64_t crossing = 0;
    for (const Edge& e : g.edges)
        if (side[e.u] != side[e.v]) ++crossing;
    return crossing;
}

inline bool isValidPartition(const Graph& g, const std::vector<char>& side) {
    if ((int)side.size() != g.n) return false;
    bool has0 = false, has1 = false;
    for (char s : side) (s ? has1 : has0) = true;
    return has0 && has1;
}
