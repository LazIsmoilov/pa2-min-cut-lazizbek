# Notes

## Karger — hand trace

Graph: two triangles joined by a bridge. Vertices 0..5.
Edges: 0-1, 1-2, 0-2 (left triangle), 3-4, 4-5, 3-5 (right triangle), 2-5 (bridge).
True min cut = 1 (just the bridge).

How I'm implementing a trial: shuffle the edge list once, then walk through it
merging endpoints with union-find, skipping any edge whose endpoints are already
in the same blob (that edge would be a self-loop). Stop at 2 blobs, then count
edges crossing between them.

This is the same as "pick a random remaining edge each time". Skipping
self-loops in a random order gives every remaining real edge the same chance of
being next, so the distribution over outcomes doesn't change. And it reuses my
PA1 union-find.

**Lucky trial.** Order starts 0-1, 4-5, 0-2, 3-5, ...
- 0-1 → {0,1} {2} {3} {4} {5}
- 4-5 → {0,1} {2} {3} {4,5}
- 0-2 → {0,1,2} {3} {4,5}
- 3-5 → {0,1,2} {3,4,5}, 2 blobs, stop
- Crossing edges: only 2-5. Cut = 1, correct.

**Unlucky trial.** Order starts 2-5, 0-1, 3-4, 1-2, ...
- 2-5 → {2,5} merged straight away, so the bridge is destroyed
- 0-1 → {0,1} {2,5} {3} {4}
- 3-4 → {0,1} {2,5} {3,4}
- 1-2 → {0,1,2,5} {3,4}, stop
- Crossing edges: 3-5, 4-5. Cut = 2, wrong.

So one trial fails exactly when the bridge gets merged before the end. That's why
you repeat trials and keep the smallest cut found.

**Edge case: disconnected graph.** Union-find never gets down to 2 blobs because
no edge joins the components. Then the answer is 0: put one component on one
side and everything else on the other.

## Stoer-Wagner — hand trace

Same graph: two triangles 0-1-2 and 3-4-5, bridge 2-5. True min cut = 1.

Idea of one phase: grow a set A one vertex at a time, always adding the vertex
with the most edge weight into A (its "key"). The last vertex added, t, has
everything else already in A, so its key is exactly the weight of the cut
"t's blob vs the rest". Record that, then merge t into the second-last vertex s.

**Phase 1** (start at 0)
- add 0. Keys: 1→1, 2→1
- add 1 (tie with 2, take first). Keys: 2→2
- add 2 (key 2). Keys: 5→1
- add 5 (key 1). Keys: 3→1, 4→1
- add 3 (key 1). Keys: 4→2
- add 4 (key 2). This is t, with s = 3.
- Cut of phase = 2: vertex 4 alone (it has 2 edges). Merge 4 into 3.

**Phase 2**: order 0, 1, 2, 5, then {3,4} last with key 2.
- Cut of phase = 2: {3,4} vs rest (edges 3-5 and 4-5). Merge {3,4} into 5.

**Phase 3**: order 0, 1, 2, then {5,3,4} last with key 1.
- Cut of phase = 1: {3,4,5} vs {0,1,2}. **This is the bridge, the real min cut.**
- Merge into 2.

**Phases 4-5** keep merging; their cuts are 2 and 2.

Best over all phases = 1, correct.

Why it works: take any min cut C. In each phase, either s and t are on
opposite sides of C, and then the phase cut is at least as good as C (the
phase cut is a minimum s-t cut), or they're on the same side and merging them
loses nothing. So some phase always catches the min cut. No randomness, and no
repeats needed. That's the deterministic guarantee Karger doesn't have.

Cost: n-1 phases, each scans all remaining vertices to find the max key for
every addition -> O(n^2) per phase, O(n^3) total. It doesn't depend on m,
which is why I predicted its runtime would look the same across graph families.
