// Regression tests for bugs found by the attack prompt (fresh chat, Oct 31).
// Each test was written and seen failing BEFORE the fix. See ai-logs/attack-karger.md.
//
// Contract: a min cut needs two non-empty sides, so n < 2 has no answer and
// must be rejected loudly (std::invalid_argument), not crash or return a fake 0.
#include <cstdio>
#include <cstdlib>
#include <stdexcept>

#include "../src/karger.hpp"
#include "../src/stoer_wagner.hpp"

static int failures = 0;

static void report(const char* name, bool ok) {
    std::printf("%-55s %s\n", name, ok ? "OK" : "FAIL");
    std::fflush(stdout);
    if (!ok) ++failures;
}

template <class F>
static bool throwsInvalidArgument(F f) {
    try {
        f();
    } catch (const std::invalid_argument&) {
        return true;
    } catch (...) {
        return false;
    }
    return false;
}

int main() {
    // Bug 2: n = 1 returned value 0 with a one-sided "partition".
    {
        Graph g{1, {}};
        report("karger: n=1 is rejected",
               throwsInvalidArgument([&] { kargerMinCut(g, 10, 1); }));
        report("stoer-wagner: n=1 is rejected",
               throwsInvalidArgument([&] { stoerWagnerMinCut(g); }));
    }

    // Bug 3b: trials = 0 returned INT64_MAX with an empty partition.
    {
        Graph g{2, {}};
        g.addEdge(0, 1);
        report("karger: trials=0 is rejected",
               throwsInvalidArgument([&] { kargerMinCut(g, 0, 1); }));
        report("karger: trials<0 is rejected",
               throwsInvalidArgument([&] { kargerMinCut(g, -5, 1); }));
    }

    // Bug 3a: trial count overflowed int64 (UB) for huge n and came out negative.
    {
        int64_t t = kargerTrialsForConfidence(950'000'000);
        report("trial count for n=9.5e8 stays positive (no overflow)", t > 0);
    }

    // Stoer-Wagner attack (limits section): an endpoint outside [0, n) is
    // accepted by addEdge and later writes outside the weight matrix.
    // A self-loop breaks the "no self-loops" contract in graph.hpp.
    // Both must be rejected when the edge is added.
    {
        report("addEdge rejects endpoint >= n", throwsInvalidArgument([] {
                   Graph g{3, {}};
                   g.addEdge(0, 3);
               }));
        report("addEdge rejects negative endpoint", throwsInvalidArgument([] {
                   Graph g{3, {}};
                   g.addEdge(-1, 2);
               }));
        report("addEdge rejects self-loop", throwsInvalidArgument([] {
                   Graph g{3, {}};
                   g.addEdge(1, 1);
               }));
    }

    // Bug 1: n = 0 segfaulted inside union-find. Run last, because before the
    // fix this crashes the whole test binary.
    {
        Graph g{0, {}};
        report("karger: n=0 is rejected",
               throwsInvalidArgument([&] { kargerMinCut(g, 1, 1); }));
        report("stoer-wagner: n=0 is rejected",
               throwsInvalidArgument([&] { stoerWagnerMinCut(g); }));
    }

    std::printf("\n%s (%d failure%s)\n", failures ? "FAILED" : "ALL PASSED",
                failures, failures == 1 ? "" : "s");
    return failures ? EXIT_FAILURE : EXIT_SUCCESS;
}
