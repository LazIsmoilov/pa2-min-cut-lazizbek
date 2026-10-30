# Times AI was wrong or misleading

## 1. Cycle as Karger's worst case (Oct 31)
**What it said:** while planning input families, Claude listed a cycle graph as Karger's worst case.
**What was wrong:** on a cycle, any final two-blob split leaves two arcs, so every run ends with cut value 2, which is the min cut. Karger can't fail there. The cycle is only the tight example for hitting one *specific* min cut (there are n(n−1)/2 of them), not for finding *a* min cut.
**What I did:** kept the cycle as an input family, but as a sanity check where Karger should succeed 100% of the time, and went looking for a real hard case instead.
## 2. Broken README command (Oct 31)
**What it said:** Claude gave me a terminal command to append a Markdown code block to the README, but wrapped it inside another code block in the chat.
**What was wrong:** the inner ``` closed the outer block early, so the chat showed the command split up and I copied an incomplete version. The terminal was left waiting on an unfinished heredoc.
**What I did:** cancelled with Ctrl+C, got a corrected command, and checked the README with `tail` before committing. Lesson: always look at a file after an AI-generated command changes it, instead of assuming it worked.
