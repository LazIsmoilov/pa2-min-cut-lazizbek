// Benchmark harness. Author: Laz
//
// Writes raw measurements (one row per repetition) to results/, so every
// table and plot can be regenerated from committed data by scripts/analyse.py.
//
// Method (report section 3):
//   - Each (family, n, instance) graph is generated ONCE and the same object is
//     handed to both algorithms. Generation is outside every timed region.
//   - Answers are checked before anything is timed: Stoer-Wagner's partition
//     must be valid and match its claimed value, must equal the planted min cut
//     where the family has one, and no Karger trial may ever find a smaller
//     cut than Stoer-Wagner (that would mean Stoer-Wagner is wrong). Any
//     failure aborts the run.
//   - One untimed warm-up run per algorithm per instance, then `reps` timed
//     repetitions with std::chrono::steady_clock.
//   - Karger is timed two ways:
//       karger_trial: a batch of trials, divided by the batch size, giving
//                     the cost of ONE trial (cheap to measure at any n).
//       karger_full:  the whole algorithm at the theoretical trial count
//                     (n choose 2) ln n, only for n <= fullKargerMaxN
//                     because it gets slow fast. Used to check that
//                     per-trial cost x trials predicts the real total.
//   - Operation counts are recorded next to times so explanations can be
//     tested: Stoer-Wagner vertex scans / pick changes, Karger edges scanned
//     per trial. Stoer-Wagner counts come from a separate untimed run.
//   - Experiments: 1 scaling with n per family, 2 vary m at fixed n,
//     3 Stoer-Wagner on the same graph with original vs relabelled vertices.
//
// Usage: ./build/bench          full run (the one in the report)
//        ./build/bench --quick  small smoke run to check everything works
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <string>
#include <vector>

#include "../src/generators.hpp"
#include "../src/karger.hpp"
#include "../src/stoer_wagner.hpp"

struct Config {
    std::vector<int> sizes;
    int reps;
    int instances;
    int successTrials;
    int fullKargerMaxN;
    int fullKargerReps;
    int varyMn;
    std::vector<int64_t> varyMm;
    int orderN;
};

static Config fullConfig() {
    return {{32, 48, 64, 96, 128, 192, 256, 384, 512, 768, 1024},
            11, 3, 1000, 128, 5, 256, {1024, 2048, 4096, 8192, 16320}, 1024};
}

static Config quickConfig() {
    return {{32, 64, 128}, 3, 2, 200, 64, 2, 64, {256, 512, 1008}, 128};
}

static volatile int64_t sink = 0;  // stops the optimiser removing timed work

template <class F>
static double timeIt(F f) {
    auto t0 = std::chrono::steady_clock::now();
    f();
    auto t1 = std::chrono::steady_clock::now();
    return std::chrono::duration<double>(t1 - t0).count();
}

[[noreturn]] static void die(const std::string& msg) {
    std::fprintf(stderr, "CHECK FAILED: %s\n", msg.c_str());
    std::exit(EXIT_FAILURE);
}

static int64_t plantedMinCut(const std::string& family, int n) {
    if (family == "cycle") return 2;
    if (family == "complete") return n - 1;
    if (family == "clusters") return 3;
    if (family == "neartie") return 5;
    return -1;  // sparse, dense, gnm: no planted value
}

static uint64_t mix(uint64_t a, uint64_t b) {
    return a * 0x9E3779B97F4A7C15ULL ^ (b + 0x632BE59BD9B4E019ULL + (a << 6) + (a >> 2));
}

// Trials per karger_trial batch: enough that one timed batch lasts a few ms.
static int batchSize(size_t m) {
    return (int)std::clamp<int64_t>(2'000'000 / ((int64_t)m + 1), 5, 2000);
}

struct Writers {
    FILE* timing;
    FILE* success;
};

static void timingRow(FILE* f, const char* experiment, const std::string& family,
                      const Graph& g, int instance, const char* algo, int rep,
                      int64_t trials, double seconds, double ops, int64_t value) {
    std::fprintf(f, "%s,%s,%d,%zu,%d,%s,%d,%lld,%.9g,%.6g,%lld\n", experiment,
                 family.c_str(), g.n, g.edges.size(), instance, algo, rep,
                 (long long)trials, seconds, ops, (long long)value);
}

// Correctness checks + success-rate experiment. Returns Stoer-Wagner's value.
static int64_t checkAndMeasureSuccess(const Graph& g, const std::string& family,
                                      int instance, int successTrials, uint64_t seed,
                                      FILE* success) {
    MinCutResult sw = stoerWagnerMinCut(g);
    std::string where = family + " n=" + std::to_string(g.n) + " instance=" +
                        std::to_string(instance);
    if (!isValidPartition(g, sw.side) || cutValue(g, sw.side) != sw.value)
        die("Stoer-Wagner returned an inconsistent partition on " + where);
    int64_t planted = plantedMinCut(family, g.n);
    if (planted >= 0 && sw.value != planted)
        die("Stoer-Wagner gave " + std::to_string(sw.value) + ", planted cut is " +
            std::to_string(planted) + " on " + where);

    std::mt19937_64 rng(seed);
    std::vector<size_t> order;
    int successes = 0;
    int64_t scanned = 0;
    for (int t = 0; t < successTrials; ++t) {
        MinCutResult r = kargerTrial(g, rng, order, &scanned);
        if (!isValidPartition(g, r.side) || cutValue(g, r.side) != r.value)
            die("Karger trial returned an inconsistent partition on " + where);
        if (r.value < sw.value)
            die("Karger found a smaller cut than Stoer-Wagner on " + where);
        if (r.value == sw.value) ++successes;
    }
    std::fprintf(success, "%s,%d,%zu,%d,%d,%d,%lld,%.6g\n", family.c_str(), g.n,
                 g.edges.size(), instance, successTrials, successes,
                 (long long)sw.value, (double)scanned / successTrials);
    return sw.value;
}

// Operation counts come from one untimed run; the timed runs pass no stats
// pointer, so counting can't affect the times. `opsPick` selects which count
// goes in the ops column (vertex scans normally, pick changes for sw_order).
static void timeStoerWagner(const Graph& g, const char* experiment,
                            const std::string& family, int instance, int reps,
                            FILE* timing, const char* algo = "stoer_wagner",
                            bool opsPick = false) {
    SWStats stats;
    sink += stoerWagnerMinCut(g, nullptr, &stats).value;  // counting run, doubles as warm-up
    const double ops = opsPick ? (double)stats.pickChanges : (double)stats.vertexScans;
    for (int rep = 0; rep < reps; ++rep) {
        int64_t value = 0;
        double s = timeIt([&] { value = stoerWagnerMinCut(g).value; });
        sink += value;
        timingRow(timing, experiment, family, g, instance, algo, rep, 1, s, ops, value);
    }
}

static void timeKargerTrial(const Graph& g, const char* experiment,
                            const std::string& family, int instance, int reps,
                            uint64_t seed, FILE* timing) {
    const int batch = batchSize(g.edges.size());
    std::vector<size_t> order;
    {  // warm-up batch
        std::mt19937_64 rng(seed);
        for (int t = 0; t < batch; ++t) sink += kargerTrial(g, rng, order).value;
    }
    for (int rep = 0; rep < reps; ++rep) {
        std::mt19937_64 rng(mix(seed, rep + 1));
        int64_t scanned = 0, best = INT64_MAX;
        double s = timeIt([&] {
            for (int t = 0; t < batch; ++t)
                best = std::min(best, kargerTrial(g, rng, order, &scanned).value);
        });
        sink += best;
        timingRow(timing, experiment, family, g, instance, "karger_trial", rep, batch,
                  s / batch, (double)scanned / batch, best);
    }
}

static void timeKargerFull(const Graph& g, const std::string& family, int instance,
                           int reps, uint64_t seed, FILE* timing) {
    const int64_t trials = kargerTrialsForConfidence(g.n);
    for (int rep = 0; rep < reps; ++rep) {
        int64_t value = 0;
        double s = timeIt([&] { value = kargerMinCut(g, trials, mix(seed, 100 + rep)).value; });
        sink += value;
        timingRow(timing, "scaling", family, g, instance, "karger_full", rep, trials, s,
                  0, value);
    }
}

int main(int argc, char** argv) {
    const bool quick = argc > 1 && std::strcmp(argv[1], "--quick") == 0;
    const Config cfg = quick ? quickConfig() : fullConfig();
    const std::string dir = quick ? "results/quick" : "results";
    std::filesystem::create_directories(dir);

    FILE* timing = std::fopen((dir + "/timing.csv").c_str(), "w");
    FILE* success = std::fopen((dir + "/success.csv").c_str(), "w");
    if (!timing || !success) die("could not open output files in " + dir);
    std::fprintf(timing, "experiment,family,n,m,instance,algo,rep,trials,seconds,ops,value\n");
    std::fprintf(success, "family,n,m,instance,trials,successes,mincut,avg_edges_scanned\n");

    const std::vector<std::string> families = {"sparse", "dense", "clusters",
                                               "neartie", "cycle", "complete"};

    // Experiment 1: scaling with n, per family.
    for (size_t fi = 0; fi < families.size(); ++fi) {
        const std::string& family = families[fi];
        for (int n : cfg.sizes) {
            // cycle and complete are fully determined by n, so one instance.
            int instances = (family == "cycle" || family == "complete") ? 1 : cfg.instances;
            for (int inst = 0; inst < instances; ++inst) {
                uint64_t seed = mix(mix(fi + 1, n), inst + 1);
                Graph g = generate(family, n, seed);
                std::fprintf(stderr, "scaling  %-9s n=%-5d m=%-7zu instance %d\n",
                             family.c_str(), n, g.edges.size(), inst);
                std::fflush(stderr);

                checkAndMeasureSuccess(g, family, inst, cfg.successTrials, mix(seed, 7),
                                       success);
                timeStoerWagner(g, "scaling", family, inst, cfg.reps, timing);
                timeKargerTrial(g, "scaling", family, inst, cfg.reps, mix(seed, 11), timing);
                if (n <= cfg.fullKargerMaxN)
                    timeKargerFull(g, family, inst, cfg.fullKargerReps, mix(seed, 13), timing);
                std::fflush(timing);
                std::fflush(success);
            }
        }
    }

    // Experiment 2: fix n, vary m. Tests whether each algorithm's cost
    // follows m the way the theory says (Stoer-Wagner: no; Karger trial: yes).
    for (int64_t m : cfg.varyMm) {
        for (int inst = 0; inst < cfg.instances; ++inst) {
            uint64_t seed = mix(mix(99, m), inst + 1);
            Graph g = gnmConnected(cfg.varyMn, m, seed);
            std::fprintf(stderr, "vary_m   gnm       n=%-5d m=%-7zu instance %d\n",
                         g.n, g.edges.size(), inst);
            std::fflush(stderr);
            checkAndMeasureSuccess(g, "gnm", inst, cfg.successTrials, mix(seed, 7), success);
            timeStoerWagner(g, "vary_m", "gnm", inst, cfg.reps, timing);
            timeKargerTrial(g, "vary_m", "gnm", inst, cfg.reps, mix(seed, 11), timing);
            std::fflush(timing);
        }
    }

    // Experiment 3: same graph, original vs randomly relabelled vertices.
    // Added after experiment 1 showed Stoer-Wagner times differing between
    // families at equal n despite identical vertex-scan counts. Relabelling
    // keeps the graph (and the work count) identical and changes only vertex
    // order, so any time difference is due to order, not structure.
    // ops column = pick changes in the find-max loop.
    for (size_t fi = 0; fi < families.size(); ++fi) {
        const std::string& family = families[fi];
        const int n = cfg.orderN;
        Graph g = generate(family, n, mix(mix(fi + 1, n), 1));  // instance 0 of experiment 1
        std::fprintf(stderr, "sw_order %-9s n=%-5d m=%-7zu\n", family.c_str(), n, g.edges.size());
        std::fflush(stderr);
        const int64_t expected = stoerWagnerMinCut(g).value;
        timeStoerWagner(g, "sw_order", family, 0, cfg.reps, timing, "sw_original", true);
        for (int inst = 0; inst < cfg.instances; ++inst) {
            Graph h = relabelRandomly(g, mix(777, inst + 1));
            if (stoerWagnerMinCut(h).value != expected)
                die("relabelling changed the min cut of " + family);
            timeStoerWagner(h, "sw_order", family, inst, cfg.reps, timing, "sw_relabelled", true);
        }
        std::fflush(timing);
    }

    std::fclose(timing);
    std::fclose(success);
    std::fprintf(stderr, "done. wrote %s/timing.csv and %s/success.csv\n", dir.c_str(),
                 dir.c_str());
    return 0;
}
