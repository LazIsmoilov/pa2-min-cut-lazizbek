# Attack prompt on Stoer-Wagner (Oct 31)

**Setup:** a second fresh chat (not the one that attacked Karger), given graph.hpp and
stoer_wagner.hpp with the tutor's prompt.

Note: it was given the version that already had the n < 2 guard from the Karger attack fix,
so it could not re-find that bug.

## Result: no bug found within the contract

- It fuzzed 400,000 random multigraphs (n = 2..9: disconnected graphs, two components, parallel
  edge bundles) under ASan + UBSan. It checked value vs brute force, `isValidPartition`, and
  `cutValue == value`. Everything passed.
- It argued through each failure category in the code: `prev`/`pick` never -1, partitions always
  two-sided, the merge step's diagonal writes are harmless, any tie-break is valid, no overflow,
  exactly n-1 phases so no hang.

## Limits it reported, and what I did

| Claim | Verdict | Action |
|---|---|---|
| Edge endpoint outside [0, n) writes out of bounds in the weight-matrix build | Real, though outside the documented contract. Nothing stopped it, and my graph generators will build edges in code | Wrote failing tests first (3 FAILs: endpoint >= n, negative endpoint, self-loop), then made `Graph::addEdge` throw `invalid_argument` |
| Self-loops are harmless (diagonal never read) | True for Stoer-Wagner, but graph.hpp says "no self-loops", so I enforce the contract in one place instead of relying on each algorithm tolerating them | Same `addEdge` check |
| n = 50,000 throws bad_alloc (20 GB matrix); n = 5,000 takes ~1e11 operations | Not bugs. This is the Theta(n^2) memory and O(n^3) time of the dense version | Used in the analysis: it bounds how large the benchmark sizes can go |
| GCC 13 `-Wstringop-overread` on `members[v] = {v}` | False positive, and not reproduced by my builds | none |

## Takeaway for the AI-use section

A clean result is still informative. A "no bugs" answer only covered the contract I gave it, and
the useful part was its list of out-of-contract inputs. I turned those into a validation step
rather than accepting "outside the contract" as good enough.
