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
