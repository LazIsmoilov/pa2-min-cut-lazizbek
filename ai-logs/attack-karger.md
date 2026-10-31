# Attack prompt on Karger (Oct 31)

**Setup:** fresh chat with no project context, given graph.hpp, union_find.hpp and karger.hpp,
plus the tutor's prompt: "Find valid inputs that make the code crash, hang, overflow, or return
a wrong answer. For each one, give the exact input, the correct output, what my code does
instead, and the line responsible."

The attacking chat compiled with AddressSanitizer + UndefinedBehaviorSanitizer and reported the
findings below. I checked each one myself before changing code: wrote `tests/test_attack.cpp`,
ran it, and confirmed it failed (output below), then fixed.

## Claimed bugs and verdicts

| # | Claim | Verified? | Fix |
|---|---|---|---|
| 1 | n = 0 segfaults: `uf.find(0)` on an empty union-find | **Real.** ASan: SEGV at union_find.hpp:20 | `kargerMinCut` throws `invalid_argument` if n < 2 |
| 2 | n = 1 returns min cut 0 with a one-sided "partition" | **Real.** My own `isValidPartition` rejects the result | same guard |
| 3 | `kargerTrialsForConfidence` overflows int64 for n >= ~9.4e8 (UB cast), giving a negative count, so 0 trials and a garbage result | **Real.** Test got a non-positive count | clamp before the cast; `kargerMinCut` throws if trials < 1 |
| 4 (minor) | `std::vector<int>` edge indices break above INT_MAX edges | Real but needs ~16 GB of edges, not testable here | switched indices to `size_t` |
| - | Occasional wrong answers / huge run times at large n | **Not bugs.** Monte Carlo failure probability <= 1/n by design; O(n^2 m log n) is the algorithm's real cost | none; this goes in the analysis |

## Found by me, not the attack

The attacking chat only saw the Karger files. Applying the same n < 2 question to Stoer-Wagner
showed the same bug there: for n = 0 or 1 the phase loop never runs and it returns INT64_MAX with
an empty partition. Same guard added, same test file covers it.

## Test output before the fix (sanitizer build)

```
karger: n=1 is rejected                                 FAIL
stoer-wagner: n=1 is rejected                           FAIL
karger: trials=0 is rejected                            FAIL
karger: trials<0 is rejected                            FAIL
trial count for n=9.5e8 stays positive (no overflow)    FAIL
tests/../src/union_find.hpp:20:27: runtime error: load of null pointer of type 'value_type'
AddressSanitizer: SEGV tests/../src/union_find.hpp:20 in UnionFind::find(int)
```

## After the fix

All attack tests pass, and the existing brute-force, Karger and Stoer-Wagner tests still pass
under ASan + UBSan.
