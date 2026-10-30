// Prints a Stoer-Wagner trace on the two-triangles + bridge graph.
// Used to write the hand-trace in notes.md.
#include <cstdio>

#include "../src/stoer_wagner.hpp"

int main() {
    Graph g{6, {}};
    g.addEdge(0, 1); g.addEdge(1, 2); g.addEdge(0, 2);
    g.addEdge(3, 4); g.addEdge(4, 5); g.addEdge(3, 5);
    g.addEdge(2, 5);
    MinCutResult r = stoerWagnerMinCut(g, stdout);
    std::printf("min cut = %lld\n", (long long)r.value);
}
