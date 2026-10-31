// Brute-force min cut: the correctness oracle.
// Author: Lazizbek
//
// Tries every way to split the vertices into two non-empty sides and keeps
// the one with the fewest crossing edges. Vertex 0 is always fixed on side 0,
// because a partition and its mirror image cut the same edges, so checking
// both would double the work for nothing. That leaves 2^(n-1) - 1 splits.
//
// Cost is O(2^n * m), so this is only for small graphs (n <= ~20). It's the
// "ground truth" every other algorithm is tested against.
#pragma once
#include <cassert>
#include <cstdint>
#include <limits>

#include "graph.hpp"

inline MinCutResult bruteForceMinCut(const Graph& g) {
    assert(g.n >= 2 && g.n <= 24);

    MinCutResult best;
    best.value = std::numeric_limits<int64_t>::max();

    // mask bit i set = vertex (i+1) is on side 1. Vertex 0 stays on side 0.
    // mask starts at 1 so side 1 is never empty.
    const uint32_t limit = 1u << (g.n - 1);
    for (uint32_t mask = 1; mask < limit; ++mask) {
        int64_t crossing = 0;
        for (const Edge& e : g.edges) {
            bool su = e.u > 0 && ((mask >> (e.u - 1)) & 1u);
            bool sv = e.v > 0 && ((mask >> (e.v - 1)) & 1u);
            if (su != sv) ++crossing;
        }
        if (crossing < best.value) {
            best.value = crossing;
            best.side.assign(g.n, 0);
            for (int i = 1; i < g.n; ++i) best.side[i] = (mask >> (i - 1)) & 1u;
        }
    }
    return best;
}
