// Generator tests: each family must produce what its comment in
// generators.hpp promises, before any timing relies on it.
//
// Checks per family: right n, right m, no duplicate edges (simple families),
// connected, deterministic for a fixed seed, and the expected min cut
// (Stoer-Wagner, plus brute force at n = 16 where it's still feasible).
#include <cstdio>
#include <cstdlib>
#include <set>
#include <utility>

#include "../src/brute_force.hpp"
#include "../src/generators.hpp"
#include "../src/karger.hpp"
#include "../src/stoer_wagner.hpp"

static int failures = 0;

static void report(const std::string& name, bool ok) {
    std::printf("%-55s %s\n", name.c_str(), ok ? "OK" : "FAIL");
    if (!ok) ++failures;
}

static bool hasDuplicateEdges(const Graph& g) {
    std::set<std::pair<int, int>> seen;
    for (const Edge& e : g.edges) {
        auto p = std::minmax(e.u, e.v);
        if (!seen.insert({p.first, p.second}).second) return true;
    }
    return false;
}

static bool sameGraph(const Graph& a, const Graph& b) {
    if (a.n != b.n || a.edges.size() != b.edges.size()) return false;
    for (size_t i = 0; i < a.edges.size(); ++i)
        if (a.edges[i].u != b.edges[i].u || a.edges[i].v != b.edges[i].v) return false;
    return true;
}

int main() {
    const std::vector<std::string> families = {"sparse", "dense", "clusters",
                                               "neartie", "cycle", "complete"};

    std::printf("== Basic shape (n = 60 and 61, seeds 1..5) ==\n");
    for (const auto& f : families) {
        bool ok = true;
        for (int n : {60, 61})
            for (uint64_t s = 1; s <= 5; ++s) {
                Graph g = generate(f, n, s);
                ok &= g.n == n && isConnected(g) && !hasDuplicateEdges(g);
                ok &= sameGraph(g, generate(f, n, s));  // deterministic
            }
        report(f + ": n, connected, simple, deterministic", ok);
    }

    std::printf("\n== Edge counts ==\n");
    report("sparse: m = 4n", generate("sparse", 100, 1).edges.size() == 400);
    report("dense: m = n(n-1)/4", generate("dense", 100, 1).edges.size() == 2475);
    report("neartie: m = 3n + 5", generate("neartie", 100, 1).edges.size() == 305);
    report("cycle: m = n", generate("cycle", 100, 1).edges.size() == 100);
    report("complete: m = n(n-1)/2", generate("complete", 100, 1).edges.size() == 4950);
    report("gnm: exact m", gnmConnected(100, 700, 3).edges.size() == 700);

    std::printf("\n== Planted min cuts (Stoer-Wagner, n = 32..200, seeds 1..5) ==\n");
    {
        bool cyc = true, comp = true, clus = true, tie = true;
        for (int n : {32, 33, 40, 100, 200})
            for (uint64_t s = 1; s <= 5; ++s) {
                cyc &= stoerWagnerMinCut(generate("cycle", n, s)).value == 2;
                comp &= stoerWagnerMinCut(generate("complete", n, s)).value == n - 1;
                clus &= stoerWagnerMinCut(generate("clusters", n, s)).value == 3;
                tie &= stoerWagnerMinCut(generate("neartie", n, s)).value == 5;
            }
        report("cycle: min cut 2", cyc);
        report("complete: min cut n-1", comp);
        report("clusters: min cut 3 (the cross edges)", clus);
        report("neartie: min cut 5 (the cross edges)", tie);
    }

    std::printf("\n== Independent checks of the planted cuts ==\n");
    {
        // neartie is small enough at n = 16..20 for brute force.
        bool ok = true;
        for (int n : {16, 17, 20})
            for (uint64_t s = 1; s <= 5; ++s)
                ok &= bruteForceMinCut(generate("neartie", n, s)).value == 5;
        report("neartie = 5 by brute force (n = 16, 17, 20)", ok);
    }
    {
        // clusters needs n >= 32, too big for brute force, so check it with
        // Karger at 2x theoretical trials (a completely different algorithm).
        bool ok = true;
        for (uint64_t s = 1; s <= 5; ++s) {
            Graph g = generate("clusters", 32, s);
            ok &= kargerMinCut(g, 2 * kargerTrialsForConfidence(32), s).value == 3;
        }
        report("clusters = 3 by Karger (n = 32)", ok);
    }
    {
        // Regression: below n = 32 the planted cut isn't guaranteed, so refuse.
        bool threw = false;
        try { generate("clusters", 16, 1); } catch (const std::invalid_argument&) { threw = true; }
        report("clusters rejects n < 32", threw);
    }

    std::printf("\n== neartie really is a near tie ==\n");
    {
        // Min degree must be 6 (only 1 above the planted cut of 5).
        Graph g = generate("neartie", 100, 1);
        std::vector<int> deg(g.n, 0);
        for (const Edge& e : g.edges) { ++deg[e.u]; ++deg[e.v]; }
        int mn = *std::min_element(deg.begin(), deg.end());
        int mx = *std::max_element(deg.begin(), deg.end());
        report("neartie: every degree is 6 or 7", mn == 6 && mx == 7);
    }

    std::printf("\n%s (%d failure%s)\n", failures ? "FAILED" : "ALL PASSED",
                failures, failures == 1 ? "" : "s");
    return failures ? EXIT_FAILURE : EXIT_SUCCESS;
}
