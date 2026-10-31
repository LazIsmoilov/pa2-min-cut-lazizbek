// Stoer-Wagner deterministic global min cut (simple O(n^3) array version).
// Primary author: Laz
//
// Works on an n x n weight matrix (parallel edges are summed into one weight).
// Each phase:
//   1. Start from any remaining vertex; that's the set A.
//   2. Repeatedly add the remaining vertex most tightly connected to A
//      (largest total edge weight into A).
//   3. Let s, t be the second-last and last vertices added. The "cut of the
//      phase" is t's weight into A at the moment it was added — this equals
//      the weight of the cut ({t's blob} vs everything else), and it's a
//      minimum s-t cut.
//   4. Record it, then merge t into s.
// After n-1 phases only one vertex is left. Every possible min cut either
// separates some phase's s and t (so that phase catches it) or keeps them
// together (so merging them was safe). The best phase cut is the global min.
//
// Cost: n-1 phases, each O(n^2) with a linear scan for the max -> O(n^3),
// regardless of how many edges the graph has.
#pragma once
#include <cstdint>
#include <cstdio>
#include <limits>
#include <stdexcept>
#include <vector>

#include "graph.hpp"

// If trace is non-null, each phase's order and cut are printed to it
// (used for the hand-trace in notes.md; off in tests and benchmarks).
// Throws std::invalid_argument if n < 2: no cut exists, and without this
// check the phase loop never runs and it returns INT64_MAX with an empty side.
inline MinCutResult stoerWagnerMinCut(const Graph& g, FILE* trace = nullptr) {
    if (g.n < 2) throw std::invalid_argument("min cut needs n >= 2");
    const int n = g.n;

    // Weight matrix, flattened. Parallel edges add up.
    std::vector<int64_t> w((size_t)n * n, 0);
    for (const Edge& e : g.edges) {
        w[(size_t)e.u * n + e.v] += 1;
        w[(size_t)e.v * n + e.u] += 1;
    }

    // members[v] = original vertices currently merged into v.
    std::vector<std::vector<int>> members(n);
    for (int v = 0; v < n; ++v) members[v] = {v};

    std::vector<int> active(n);  // vertices not yet merged away
    for (int v = 0; v < n; ++v) active[v] = v;

    MinCutResult best;
    best.value = std::numeric_limits<int64_t>::max();

    std::vector<int64_t> key(n);
    std::vector<char> inA(n);

    for (int phase = 1; (int)active.size() > 1; ++phase) {
        for (int v : active) { key[v] = 0; inA[v] = 0; }

        int prev = -1, last = -1;
        int64_t cutOfPhase = 0;
        if (trace) std::fprintf(trace, "phase %d order:", phase);

        for (size_t added = 0; added < active.size(); ++added) {
            // Pick the not-yet-added vertex with the largest key.
            // Ties go to whichever comes first in `active`.
            int pick = -1;
            for (int v : active)
                if (!inA[v] && (pick == -1 || key[v] > key[pick])) pick = v;

            inA[pick] = 1;
            prev = last;
            last = pick;
            cutOfPhase = key[pick];
            if (trace) std::fprintf(trace, " %d(key %lld)", pick, (long long)key[pick]);

            for (int v : active)
                if (!inA[v]) key[v] += w[(size_t)pick * n + v];
        }

        if (trace) {
            std::fprintf(trace, "\n  s=%d t=%d, cut of phase = %lld, t's blob = {", prev,
                         last, (long long)cutOfPhase);
            for (size_t i = 0; i < members[last].size(); ++i)
                std::fprintf(trace, "%s%d", i ? "," : "", members[last][i]);
            std::fprintf(trace, "}\n");
        }

        if (cutOfPhase < best.value) {
            best.value = cutOfPhase;
            best.side.assign(n, 0);
            for (int v : members[last]) best.side[v] = 1;
        }

        // Merge last (t) into prev (s).
        for (int v : active) {
            w[(size_t)prev * n + v] += w[(size_t)last * n + v];
            w[(size_t)v * n + prev] = w[(size_t)prev * n + v];
        }
        w[(size_t)prev * n + prev] = 0;
        members[prev].insert(members[prev].end(), members[last].begin(),
                             members[last].end());
        for (size_t i = 0; i < active.size(); ++i)
            if (active[i] == last) { active.erase(active.begin() + i); break; }
    }
    return best;
}
