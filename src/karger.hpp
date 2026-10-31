// Karger's randomized contraction algorithm for global min cut.
// Primary author: Lazizbek
//
// One trial: shuffle the edges, then walk through them merging endpoints
// with union-find until only 2 blobs are left. Edges whose endpoints are
// already in the same blob are skipped (they'd be self-loops). This gives
// the same outcome distribution as "pick a uniformly random remaining edge
// and contract it", because skipping self-loops in a random order leaves
// every remaining real edge equally likely to come next.
//
// One trial costs O(m) for the shuffle + O(m * alpha(n)) for contraction.
// A single trial finds a specific min cut with probability >= 2/(n(n-1)),
// so we repeat and keep the best.
#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <random>
#include <stdexcept>
#include <vector>

#include "graph.hpp"
#include "union_find.hpp"

// Number of trials so a specific min cut survives at least once with
// probability >= 1 - 1/n:  (n choose 2) * ln n, rounded up.
// Clamped before the cast: for n around 9.4e8 the value exceeds INT64_MAX,
// and casting an out-of-range double to int64 is undefined behaviour
// (found by the attack prompt, see ai-logs/attack-karger.md).
inline int64_t kargerTrialsForConfidence(int n) {
    if (n <= 2) return 1;
    double pairs = 0.5 * n * (n - 1.0);
    double trials = std::ceil(pairs * std::log((double)n));
    if (trials >= 9.2e18) return std::numeric_limits<int64_t>::max();
    return (int64_t)trials;
}

// One contraction trial. `order` is scratch space reused across trials so we
// don't reallocate every time.
// Edge indices are size_t so graphs with more than INT_MAX edges don't
// overflow the index (attack prompt, minor finding).
// If edgesScanned is non-null, the number of edges looked at before reaching
// 2 blobs is added to it (used by the benchmark to test explanations; it
// doesn't change what the trial does).
inline MinCutResult kargerTrial(const Graph& g, std::mt19937_64& rng,
                                std::vector<size_t>& order,
                                int64_t* edgesScanned = nullptr) {
    order.resize(g.edges.size());
    for (size_t i = 0; i < order.size(); ++i) order[i] = i;
    std::shuffle(order.begin(), order.end(), rng);

    UnionFind uf(g.n);
    int64_t scanned = 0;
    for (size_t idx : order) {
        if (uf.components() == 2) break;
        const Edge& e = g.edges[idx];
        uf.unite(e.u, e.v);  // no-op if already merged (self-loop)
        ++scanned;
    }
    if (edgesScanned) *edgesScanned += scanned;

    // If the graph is disconnected we may finish with more than 2 blobs.
    // Putting vertex 0's blob on side 0 and everything else on side 1 then
    // gives a cut of 0, which is correct. With exactly 2 blobs it's just the
    // normal split.
    MinCutResult r;
    r.side.assign(g.n, 0);
    int root0 = uf.find(0);
    for (int i = 0; i < g.n; ++i) r.side[i] = (uf.find(i) == root0) ? 0 : 1;
    r.value = cutValue(g, r.side);
    return r;
}

// Repeat `trials` independent trials and keep the smallest cut.
// Throws std::invalid_argument if n < 2 (no cut exists) or trials < 1.
inline MinCutResult kargerMinCut(const Graph& g, int64_t trials, uint64_t seed) {
    if (g.n < 2) throw std::invalid_argument("min cut needs n >= 2");
    if (trials < 1) throw std::invalid_argument("karger needs trials >= 1");
    std::mt19937_64 rng(seed);
    std::vector<size_t> order;
    MinCutResult best;
    best.value = std::numeric_limits<int64_t>::max();
    for (int64_t t = 0; t < trials; ++t) {
        MinCutResult r = kargerTrial(g, rng, order);
        if (r.value < best.value) {
            best = std::move(r);
            if (best.value == 0) break;  // can't do better than 0
        }
    }
    return best;
}
