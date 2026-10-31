// Union-find with path compression + union by rank.
// Author: Lazizbek (same structure as my PA1 optimised union-find)
//
// Used by Karger to track which original vertices have been merged into
// the same blob. components() is kept up to date so a trial knows when it
// has reached 2 blobs without rescanning.
#pragma once
#include <numeric>
#include <utility>
#include <vector>

class UnionFind {
public:
    explicit UnionFind(int n) : parent_(n), rank_(n, 0), components_(n) {
        std::iota(parent_.begin(), parent_.end(), 0);
    }

    int find(int x) {
        // Path compression: point every node on the way up at the root.
        while (parent_[x] != x) {
            parent_[x] = parent_[parent_[x]];
            x = parent_[x];
        }
        return x;
    }

    // Returns true if a and b were in different sets (a real merge happened).
    bool unite(int a, int b) {
        a = find(a);
        b = find(b);
        if (a == b) return false;
        if (rank_[a] < rank_[b]) std::swap(a, b);
        parent_[b] = a;
        if (rank_[a] == rank_[b]) ++rank_[a];
        --components_;
        return true;
    }

    int components() const { return components_; }

private:
    std::vector<int> parent_;
    std::vector<int> rank_;
    int components_;
};
