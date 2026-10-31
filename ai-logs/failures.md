# Times AI was wrong or misleading

## 1. Cycle as Karger's worst case (Oct 31)
**What it said:** while planning input families, Claude listed a cycle graph as Karger's worst case.
**What was wrong:** on a cycle, any final two-blob split leaves two arcs, so every run ends with cut value 2, which is the min cut. Karger can't fail there. The cycle is only the tight example for hitting one *specific* min cut (there are n(n−1)/2 of them), not for finding *a* min cut.
**What I did:** kept the cycle as an input family, but as a sanity check where Karger should succeed 100% of the time, and went looking for a real hard case instead.
## 2. Broken README command (Oct 31)
**What it said:** Claude gave me a terminal command to append a Markdown code block to the README, but wrapped it inside another code block in the chat.
**What was wrong:** the inner ``` closed the outer block early, so the chat showed the command split up and I copied an incomplete version. The terminal was left waiting on an unfinished heredoc.
**What I did:** cancelled with Ctrl+C, got a corrected command, and checked the README with `tail` before committing. Lesson: always look at a file after an AI-generated command changes it, instead of assuming it worked.

## 3. Clusters generator didn't guarantee its own planted cut (Oct 31)
**What it said:** Claude wrote a "clusters" generator (two dense random halves joined by 3 cross edges) with a comment saying the min cut is always those 3 cross edges.
**What was wrong:** the code never enforced that. At n = 16-20 the random halves were sparse enough that some vertex had only 1-2 edges, so cutting that single vertex off was cheaper. Brute force gave min cuts of 1 and 2, not 3. If the experiments had used it unchecked, every "clusters" result would have been measuring a different cut from the one I thought I'd planted.
**How it was caught:** the generator test compares each planted cut against brute force at n = 16. That check failed.
**What I did:** the generator now rejects any draw where a vertex has degree < 4 and refuses n < 32. Rechecked with Stoer-Wagner (n = 32-200), with Karger at n = 32, and with a stress run of 1,960 graphs (n = 32-80, 20 seeds each): all give 3. Lesson: a generator's comment is a claim, so test the claim, not just the shape of the output.
