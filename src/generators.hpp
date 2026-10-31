// Input families for the experiments. Author: Laz
//
// Every generator is deterministic given (n, seed), so the harness can hand
// the exact same instance to both algorithms and anyone can regenerate it.
//
// Families and why each one is here:
//   sparse    random simple graph, m = 4n (average degree 8), connected.
//             Karger's per-trial cost is O(m), so this is its best case.
//   dense     random simple graph, m = n(n-1)/4 (edge probability 1/2), connected.
//             Karger pays for every edge; Stoer-Wagner's O(n^3) is unchanged.
//   clusters  two dense halves (p = 1/2 inside) joined by 3 random cross edges.
//             The min cut is the 3 cross edges, not a single vertex.
//   neartie   two ring lattices of degree 6 joined by 5 cross edges.
//             Min cut = 5 (planted, balanced), but every vertex has degree
//             6 or 7, so there are n "almost min" cuts competing with it.
//             Built to be hard for Karger: a trial fails if it ever
//             contracts one of the 5 cross edges.
//   cycle     n-cycle. Min cut 2, and every two-arc split achieves it,
//             so Karger can never fail (sanity check).
//   complete  K_n. Min cut n-1 (one vertex alone). Densest possible input.
//   gnm       random simple graph with exactly m edges, connected. Used to
//             vary m at fixed n (tests "Stoer-Wagner ignores m").
#pragma once
#include <algorithm>
#include <cstdint>
#include <random>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <vector>

#include "graph.hpp"
#include "union_find.hpp"

inline bool isConnected(const Graph& g) {
    UnionFind uf(g.n);
    for (const Edge& e : g.edges) uf.unite(e.u, e.v);
    return uf.components() == 1;
}

// Random simple graph on vertices [offset, offset+n) with exactly m distinct
// edges, added to g. Requires m <= n(n-1)/2.
inline void addRandomSimpleEdges(Graph& g, int offset, int n, int64_t m,
                                 std::mt19937_64& rng) {
    const int64_t total = (int64_t)n * (n - 1) / 2;
    if (m > total) throw std::invalid_argument("too many edges for a simple graph");
    std::uniform_int_distribution<int> pick(0, n - 1);
    std::unordered_set<uint64_t> used;
    used.reserve((size_t)m * 2);
    int64_t added = 0;
    while (added < m) {
        int a = pick(rng), b = pick(rng);
        if (a == b) continue;
        if (a > b) std::swap(a, b);
        uint64_t key = (uint64_t)a * (uint64_t)n + (uint64_t)b;
        if (!used.insert(key).second) continue;
        g.addEdge(offset + a, offset + b);
        ++added;
    }
}

// Keep drawing until connected. Seeds are derived, so still deterministic.
inline Graph gnmConnected(int n, int64_t m, uint64_t seed) {
    for (uint64_t attempt = 0; attempt < 1000; ++attempt) {
        std::mt19937_64 rng(seed * 1000003ULL + attempt);
        Graph g{n, {}};
        addRandomSimpleEdges(g, 0, n, m, rng);
        if (isConnected(g)) return g;
    }
    throw std::runtime_error("could not generate a connected graph");
}

inline Graph genSparse(int n, uint64_t seed) { return gnmConnected(n, 4LL * n, seed); }

inline Graph genDense(int n, uint64_t seed) {
    return gnmConnected(n, (int64_t)n * (n - 1) / 4, seed);
}

// Cross edges join distinct vertex pairs so no single vertex absorbs them.
inline void addCrossEdges(Graph& g, int half, int k, std::mt19937_64& rng) {
    std::vector<int> left(half), right(g.n - half);
    for (int i = 0; i < half; ++i) left[i] = i;
    for (int i = 0; i < g.n - half; ++i) right[i] = half + i;
    std::shuffle(left.begin(), left.end(), rng);
    std::shuffle(right.begin(), right.end(), rng);
    for (int i = 0; i < k; ++i) g.addEdge(left[i], right[i]);
}

inline int minDegree(const Graph& g) {
    std::vector<int> deg(g.n, 0);
    for (const Edge& e : g.edges) { ++deg[e.u]; ++deg[e.v]; }
    return *std::min_element(deg.begin(), deg.end());
}

// The planted cut (3 cross edges) is only the minimum if no vertex can be cut
// off more cheaply, so draws with any vertex of degree < 4 are rejected.
// Without this, small n gave min cuts of 1 or 2 (a lonely vertex), not 3.
// n >= 32 so the halves are dense enough for that to be reachable.
inline Graph genClusters(int n, uint64_t seed) {
    if (n < 32) throw std::invalid_argument("clusters needs n >= 32");
    for (uint64_t attempt = 0; attempt < 1000; ++attempt) {
        std::mt19937_64 rng(seed * 1000003ULL + attempt);
        Graph g{n, {}};
        int half = n / 2;
        addRandomSimpleEdges(g, 0, half, (int64_t)half * (half - 1) / 4, rng);
        int other = n - half;
        addRandomSimpleEdges(g, half, other, (int64_t)other * (other - 1) / 4, rng);
        addCrossEdges(g, half, 3, rng);
        if (isConnected(g) && minDegree(g) >= 4) return g;
    }
    throw std::runtime_error("could not generate a valid clusters graph");
}

// Ring lattice: each vertex joined to its 3 nearest neighbours on each side.
inline void addRingLattice(Graph& g, int offset, int n) {
    for (int i = 0; i < n; ++i)
        for (int d = 1; d <= 3; ++d) g.addEdge(offset + i, offset + (i + d) % n);
}

inline Graph genNearTie(int n, uint64_t seed) {
    if (n < 16) throw std::invalid_argument("neartie needs n >= 16");
    std::mt19937_64 rng(seed);
    Graph g{n, {}};
    int half = n / 2;
    addRingLattice(g, 0, half);
    addRingLattice(g, half, n - half);
    addCrossEdges(g, half, 5, rng);
    return g;
}

inline Graph genCycle(int n) {
    Graph g{n, {}};
    for (int i = 0; i < n; ++i) g.addEdge(i, (i + 1) % n);
    return g;
}

inline Graph genComplete(int n) {
    Graph g{n, {}};
    for (int i = 0; i < n; ++i)
        for (int j = i + 1; j < n; ++j) g.addEdge(i, j);
    return g;
}

inline Graph generate(const std::string& family, int n, uint64_t seed) {
    if (family == "sparse") return genSparse(n, seed);
    if (family == "dense") return genDense(n, seed);
    if (family == "clusters") return genClusters(n, seed);
    if (family == "neartie") return genNearTie(n, seed);
    if (family == "cycle") return genCycle(n);
    if (family == "complete") return genComplete(n);
    throw std::invalid_argument("unknown family: " + family);
}
