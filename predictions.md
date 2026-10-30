# Predictions (written before any implementation or measurement)

Drafted with AI help, then checked and agreed to by me before committing. Not to be edited after experiments run.

## Costs on paper
- Karger, one trial: shuffle + union-find contraction, roughly O(m).
- Trials needed so a specific min cut is found with prob ≥ 1 − 1/n: about (n(n−1)/2)·ln n.
  At n = 50 that's 1225 · 3.9 ≈ 4,800 trials. One trial's guaranteed success chance is only 1/1225 ≈ 0.08%.
- So Karger at theoretical confidence is about O(n² · m · log n). Dense graphs (m ~ n²) make that roughly n⁴ log n.
- Stoer-Wagner (array version): O(n³), and it barely cares about m.

## What I expect
1. **If Karger runs the theoretical trial count, Stoer-Wagner wins basically everywhere above tiny n.** I don't expect a real crossover in that setup.
2. **The theoretical bound is very pessimistic.** On random graphs I expect one trial to succeed far more often than 0.08%, maybe several percent or more, because the min cut is often just the lowest-degree vertex, which is hard to "accidentally" destroy. If that's true, Karger with a *measured* trial count could actually beat Stoer-Wagner on sparse graphs.
3. **Stoer-Wagner's runtime should look almost identical across graph families at the same n**, since the array version does the same n³ work regardless of structure.
4. **Karger gets hurt most** when the true min cut isn't a single vertex and there are lots of near-minimum cuts competing with it, e.g. a planted balanced cut only slightly smaller than the minimum degree.
5. **Cycle graphs:** Karger should succeed on 100% of trials.