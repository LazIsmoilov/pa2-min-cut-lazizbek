"""Turns the committed raw data into every table and figure in the report.

Author: Laz. Run from the repo root:  python3 scripts/analyse.py
Reads   results/timing.csv, results/success.csv
Writes  figures/*.png, results/tables.md, results/key_numbers.json

Nothing in the report is typed by hand: if a number is in the report, it
comes from tables.md or key_numbers.json, which come from this script.

Conventions
- Times: median over all repetitions x instances; spread = interquartile
  range (25th-75th percentile), drawn as error bars.
- Karger single-trial success rate p: pooled over all trials of all instances
  of a (family, n), with a 95% Wilson interval.
- "Trials needed" for both the theoretical and the measured version use the
  same target: find the min cut with probability >= 1 - 1/n.
    theory:   (n choose 2) * ln n          (from p >= 2 / (n(n-1)))
    measured: ln n / -ln(1 - p_hat)        (solves (1-p)^k = 1/n)
- Projected Karger cost = median per-trial time x trials needed. The
  projection is checked against real full runs (karger_full) for n <= 128.
"""

import json
import math
import os

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt  # noqa: E402
import numpy as np  # noqa: E402
import pandas as pd  # noqa: E402

FAMILIES = ["sparse", "dense", "clusters", "neartie", "cycle", "complete"]
# Fixed categorical order (validated palette), plus a marker per family so
# identity never depends on colour alone.
COLOR = dict(zip(FAMILIES, ["#2a78d6", "#eb6834", "#1baf7a", "#eda100", "#e87ba4", "#008300"]))
MARKER = dict(zip(FAMILIES, ["o", "s", "^", "D", "v", "P"]))
INK, INK2, GRID = "#0b0b0b", "#52514e", "#e4e3df"
ALGO_COLOR = {"stoer_wagner": "#2a78d6", "theory": "#eb6834", "measured": "#1baf7a"}

plt.rcParams.update({
    "figure.dpi": 150, "savefig.dpi": 200, "font.size": 9,
    "axes.edgecolor": INK2, "axes.labelcolor": INK, "xtick.color": INK2,
    "ytick.color": INK2, "axes.grid": True, "grid.color": GRID,
    "grid.linewidth": 0.6, "axes.spines.top": False, "axes.spines.right": False,
    "lines.linewidth": 2, "lines.markersize": 5, "legend.frameon": False,
    "figure.facecolor": "white", "axes.facecolor": "white",
})


def q25(x):
    return x.quantile(0.25)


def q75(x):
    return x.quantile(0.75)


def wilson(k, n, z=1.96):
    if n == 0:
        return (0.0, 0.0)
    p = k / n
    d = 1 + z * z / n
    c = (p + z * z / (2 * n)) / d
    h = z * math.sqrt(p * (1 - p) / n + z * z / (4 * n * n)) / d
    return (max(0.0, c - h), min(1.0, c + h))


def trials_theory(n):
    return n * (n - 1) / 2 * math.log(n)


def trials_measured(n, p):
    if p <= 0:
        return math.inf
    if p >= 1:
        return 1.0
    return max(1.0, math.log(n) / -math.log1p(-p))


def summarise_timing(t):
    return (t.groupby(["experiment", "family", "n", "m", "algo"])
             .agg(med=("seconds", "median"), lo=("seconds", q25), hi=("seconds", q75),
                  ops=("ops", "median"), trials=("trials", "median"))
             .reset_index())


def summarise_success(s):
    g = (s.groupby(["family", "n"])
          .agg(m=("m", "mean"), trials=("trials", "sum"), successes=("successes", "sum"),
               scanned=("avg_edges_scanned", "mean"))
          .reset_index())
    g["p"] = g.successes / g.trials
    ci = [wilson(k, n) for k, n in zip(g.successes, g.trials)]
    g["p_lo"] = [c[0] for c in ci]
    g["p_hi"] = [c[1] for c in ci]
    g["bound"] = 2 / (g.n * (g.n - 1))
    g["ratio"] = g.p / g.bound
    g["k_theory"] = [trials_theory(n) for n in g.n]
    g["k_measured"] = [trials_measured(n, p) for n, p in zip(g.n, g.p)]
    return g


def build_costs(ts, ss):
    sc = ts[ts.experiment == "scaling"]
    sw = sc[sc.algo == "stoer_wagner"][["family", "n", "med", "lo", "hi", "ops"]]
    sw = sw.rename(columns={"med": "sw", "lo": "sw_lo", "hi": "sw_hi", "ops": "sw_scans"})
    kt = sc[sc.algo == "karger_trial"][["family", "n", "med", "lo", "hi"]]
    kt = kt.rename(columns={"med": "trial", "lo": "trial_lo", "hi": "trial_hi"})
    x = sw.merge(kt, on=["family", "n"]).merge(ss, on=["family", "n"])
    x["karger_theory"] = x.trial * x.k_theory
    x["karger_measured"] = x.trial * x.k_measured
    x["winner_measured"] = np.where(x.karger_measured < x.sw, "Karger", "Stoer-Wagner")
    return x


def fig_success(ss, out):
    fig, ax = plt.subplots(figsize=(6.4, 3.8))
    for f in FAMILIES:
        d = ss[ss.family == f].sort_values("n")
        # p = 1 (cycle) can't sit on a log axis at exactly 1 with an interval; it's fine.
        yerr = [d.p - d.p_lo, d.p_hi - d.p]
        ax.errorbar(d.n, d.p, yerr=yerr, color=COLOR[f], marker=MARKER[f], label=f,
                    capsize=2, elinewidth=1)
    nn = np.array(sorted(ss.n.unique()))
    ax.plot(nn, 2 / (nn * (nn - 1)), color=INK2, linestyle="--", linewidth=1.5,
            label="theoretical lower bound 2/(n(n-1))")
    ax.set_xscale("log", base=2)
    ax.set_yscale("log")
    ax.set_xlabel("n (vertices)")
    ax.set_ylabel("P(one Karger trial finds the min cut)")
    ax.set_title("Measured single-trial success rate vs the guaranteed bound", loc="left")
    ax.legend(fontsize=7.5, ncol=2, loc="lower left")
    fig.tight_layout()
    fig.savefig(out)
    plt.close(fig)


def fig_sw_by_family(x, out):
    fig, ax = plt.subplots(figsize=(6.4, 3.8))
    for f in FAMILIES:
        d = x[x.family == f].sort_values("n")
        ax.errorbar(d.n, d.sw, yerr=[d.sw - d.sw_lo, d.sw_hi - d.sw], color=COLOR[f],
                    marker=MARKER[f], label=f, capsize=2, elinewidth=1)
    ax.set_xscale("log", base=2)
    ax.set_yscale("log")
    ax.set_xlabel("n (vertices)")
    ax.set_ylabel("Stoer-Wagner time (s)")
    ax.set_title("Stoer-Wagner time is the same for every family at a given n", loc="left")
    ax.legend(fontsize=7.5, ncol=3, loc="upper left")
    fig.tight_layout()
    fig.savefig(out)
    plt.close(fig)


def fig_total_cost(x, ts, out):
    full = ts[(ts.experiment == "scaling") & (ts.algo == "karger_full")]
    fig, axes = plt.subplots(2, 3, figsize=(8.6, 5.4), sharex=True, sharey=True)
    for ax, f in zip(axes.flat, FAMILIES):
        d = x[x.family == f].sort_values("n")
        ax.plot(d.n, d.sw, color=ALGO_COLOR["stoer_wagner"], marker="o", label="Stoer-Wagner")
        ax.plot(d.n, d.karger_theory, color=ALGO_COLOR["theory"], marker="s",
                label="Karger, theoretical trial count (projected)")
        ax.plot(d.n, d.karger_measured, color=ALGO_COLOR["measured"], marker="^",
                label="Karger, measured trial count (projected)")
        fd = full[full.family == f]
        ax.scatter(fd.n, fd.med, s=46, facecolors="none", edgecolors=INK, linewidths=1.2,
                   zorder=5, label="Karger, theoretical count (actually run)")
        ax.set_title(f, loc="left", fontsize=9)
        ax.set_xscale("log", base=2)
        ax.set_yscale("log")
    for ax in axes[1]:
        ax.set_xlabel("n (vertices)")
    for ax in axes[:, 0]:
        ax.set_ylabel("time to min cut (s)")
    h, l = axes[0, 0].get_legend_handles_labels()
    fig.legend(h, l, loc="lower center", ncol=2, fontsize=7.5, bbox_to_anchor=(0.5, -0.01))
    fig.suptitle("Total cost to reach the min cut with probability >= 1 - 1/n",
                 x=0.01, ha="left", fontsize=10)
    fig.tight_layout(rect=(0, 0.08, 1, 0.97))
    fig.savefig(out)
    plt.close(fig)


def fig_vary_m(ts, s, out):
    v = ts[ts.experiment == "vary_m"].sort_values("m")
    sw = v[v.algo == "stoer_wagner"].set_index("m")
    kt = v[v.algo == "karger_trial"].set_index("m")
    scan = s[s.family == "gnm"].groupby("m").avg_edges_scanned.mean()
    m = sw.index.values
    base = m[0]
    fig, ax = plt.subplots(figsize=(6.4, 3.8))
    ax.plot(m, m / base, color=INK2, linestyle="--", linewidth=1.5,
            label="proportional to m (reference)")
    ax.plot(m, sw.med / sw.med.iloc[0], color=ALGO_COLOR["stoer_wagner"], marker="o",
            label="Stoer-Wagner time")
    ax.plot(m, sw.ops / sw.ops.iloc[0], color="#6250d6", marker="x",
            label="Stoer-Wagner vertex scans")
    ax.plot(m, kt.med / kt.med.iloc[0], color=ALGO_COLOR["theory"], marker="s",
            label="Karger time per trial")
    ax.plot(m, scan.loc[m] / scan.loc[base], color=ALGO_COLOR["measured"], marker="^",
            label="Karger edges scanned per trial")
    ax.set_xscale("log", base=2)
    ax.set_yscale("log", base=2)
    ax.set_xlabel(f"m (edges), n fixed at {int(v.n.iloc[0])}")
    ax.set_ylabel(f"relative to m = {base} (x)")
    ax.set_title("What actually grows with m", loc="left")
    ax.legend(fontsize=7.5, loc="upper left")
    fig.tight_layout()
    fig.savefig(out)
    plt.close(fig)


def fig_sw_order(t, out):
    o = t[t.experiment == "sw_order"]
    fig, ax = plt.subplots(figsize=(6.4, 3.6))
    xs = np.arange(len(FAMILIES))
    width = 0.36
    for i, (algo, label, color) in enumerate([
            ("sw_original", "original vertex numbering", ALGO_COLOR["stoer_wagner"]),
            ("sw_relabelled", "same graph, randomly relabelled", ALGO_COLOR["theory"])]):
        meds, los, his = [], [], []
        for f in FAMILIES:
            d = o[(o.family == f) & (o.algo == algo)].seconds
            meds.append(d.median()); los.append(d.quantile(0.25)); his.append(d.quantile(0.75))
        meds, los, his = map(np.array, (meds, los, his))
        ax.bar(xs + (i - 0.5) * (width + 0.02), meds, width, color=color, label=label,
               yerr=[meds - los, his - meds], capsize=2, error_kw={"elinewidth": 1})
    ax.set_xticks(xs, FAMILIES)
    ax.set_ylabel("Stoer-Wagner time (s)")
    ax.set_title(f"Same graph, same work, different vertex order (n = {int(o.n.iloc[0])})",
                 loc="left")
    ax.legend(fontsize=7.5, loc="upper right")
    ax.grid(axis="x", visible=False)
    fig.tight_layout()
    fig.savefig(out)
    plt.close(fig)


def fmt_s(v):
    if not np.isfinite(v):
        return "inf"
    if v >= 3600:
        return f"{v / 3600:.1f} h"
    if v >= 60:
        return f"{v / 60:.1f} min"
    if v >= 1:
        return f"{v:.2f} s"
    if v >= 1e-3:
        return f"{v * 1e3:.2f} ms"
    return f"{v * 1e6:.1f} us"


def main():
    t = pd.read_csv("results/timing.csv")
    s = pd.read_csv("results/success.csv")
    os.makedirs("figures", exist_ok=True)

    ts = summarise_timing(t)
    ss = summarise_success(s[s.family != "gnm"])
    x = build_costs(ts, ss)

    fig_success(ss, "figures/fig1_success_rate.png")
    fig_sw_by_family(x, "figures/fig2_stoer_wagner_by_family.png")
    fig_total_cost(x, ts, "figures/fig3_total_cost.png")
    fig_vary_m(ts, s, "figures/fig4_vary_m.png")
    fig_sw_order(t, "figures/fig5_sw_vertex_order.png")

    nmax = x.n.max()
    lines = ["# Tables (generated by scripts/analyse.py, do not edit by hand)", ""]

    lines += [f"## Table 1. Head-to-head at n = {nmax}", "",
              "| family | m | min cut | Stoer-Wagner | Karger / trial | p (95% CI) | bound 2/(n(n-1)) | p / bound | trials: theory | trials: measured | Karger total: theory | Karger total: measured | faster (measured) |",
              "|---|---|---|---|---|---|---|---|---|---|---|---|---|"]
    mincut = s.groupby(["family", "n"]).mincut.median()
    for f in FAMILIES:
        r = x[(x.family == f) & (x.n == nmax)].iloc[0]
        lines.append(
            f"| {f} | {int(r.m)} | {int(mincut[(f, nmax)])} | {fmt_s(r.sw)} | {fmt_s(r.trial)} | "
            f"{r.p:.3f} ({r.p_lo:.3f}-{r.p_hi:.3f}) | {r.bound:.2e} | {r.ratio:,.0f} | "
            f"{r.k_theory:,.0f} | {r.k_measured:,.1f} | {fmt_s(r.karger_theory)} | "
            f"{fmt_s(r.karger_measured)} | {r.winner_measured} |")

    lines += ["", "## Table 2. Does per-trial time x trials predict a real full Karger run?", "",
              "| family | n | trials | projected | actually measured (median) | measured / projected |",
              "|---|---|---|---|---|---|"]
    full = ts[(ts.experiment == "scaling") & (ts.algo == "karger_full")]
    ratios = []
    for _, r in full.sort_values(["family", "n"]).iterrows():
        xr = x[(x.family == r.family) & (x.n == r.n)].iloc[0]
        proj = xr.trial * r.trials
        ratios.append(r.med / proj)
        lines.append(f"| {r.family} | {int(r.n)} | {int(r.trials):,} | {fmt_s(proj)} | "
                     f"{fmt_s(r.med)} | {r.med / proj:.2f} |")

    lines += ["", "## Table 3. Fix n, vary m", "",
              "| m | Stoer-Wagner time (IQR) | SW vertex scans | Karger time / trial (IQR) | Karger edges scanned / trial | edges scanned as % of m |",
              "|---|---|---|---|---|---|"]
    v = ts[ts.experiment == "vary_m"]
    scan = s[s.family == "gnm"].groupby("m").avg_edges_scanned.mean()
    for m_ in sorted(v.m.unique()):
        a = v[(v.m == m_) & (v.algo == "stoer_wagner")].iloc[0]
        b = v[(v.m == m_) & (v.algo == "karger_trial")].iloc[0]
        lines.append(f"| {int(m_)} | {fmt_s(a.med)} ({fmt_s(a.lo)}-{fmt_s(a.hi)}) | {int(a.ops):,} | "
                     f"{fmt_s(b.med)} ({fmt_s(b.lo)}-{fmt_s(b.hi)}) | {scan[m_]:.0f} | "
                     f"{100 * scan[m_] / m_:.1f}% |")

    lines += ["", "## Table 4. Karger single-trial success rate, all sizes", "",
              "| family | " + " | ".join(str(n) for n in sorted(ss.n.unique())) + " |",
              "|---|" + "---|" * ss.n.nunique()]
    for f in FAMILIES:
        d = ss[ss.family == f].sort_values("n")
        lines.append(f"| {f} | " + " | ".join(f"{p:.3f}" for p in d.p) + " |")

    lines += ["", "## Table 5. Edges a Karger trial scans before stopping, as % of m", "",
              "| family | " + " | ".join(str(n) for n in sorted(ss.n.unique())) + " |",
              "|---|" + "---|" * ss.n.nunique()]
    for f in FAMILIES:
        d = ss[ss.family == f].sort_values("n")
        lines.append(f"| {f} | " + " | ".join(f"{100 * sc / m:.1f}" for sc, m in zip(d.scanned, d.m)) + " |")

    lines += ["", "## Table 6. Testing the explanation for Karger's success rate on planted cuts", "",
              "Model: a trial succeeds iff none of the k planted cross edges falls in the",
              "first S positions of the shuffled order, where S = measured edges scanned",
              "per trial. Predicted p = (1 - S/m)^k. Compared with the measured p and its 95% CI.", "",
              "| family | k | n | m | S (edges scanned) | S / m | predicted p | measured p (95% CI) | inside CI? |",
              "|---|---|---|---|---|---|---|---|---|"]
    model_hits, model_total = 0, 0
    for f, k in (("clusters", 3), ("neartie", 5)):
        for _, r in ss[ss.family == f].sort_values("n").iterrows():
            pred = (1 - r.scanned / r.m) ** k
            inside = r.p_lo <= pred <= r.p_hi
            model_hits += inside
            model_total += 1
            lines.append(f"| {f} | {k} | {int(r.n)} | {int(r.m)} | {r.scanned:.0f} | {r.scanned / r.m:.3f} | "
                         f"{pred:.3f} | {r.p:.3f} ({r.p_lo:.3f}-{r.p_hi:.3f}) | {'yes' if inside else 'no'} |")
    lines += ["", f"Model prediction inside the measured 95% CI in {model_hits} of {model_total} rows."]

    lines += ["", "## Table 7. Stoer-Wagner: same graph, original vs relabelled vertex order", "",
              "| family | original: time (IQR) | original: pick changes | relabelled: time (IQR) | relabelled: pick changes (mean of instances) | time ratio relabelled / original |",
              "|---|---|---|---|---|---|"]
    o = t[t.experiment == "sw_order"]
    order_rows = {}
    for f in FAMILIES:
        a = o[(o.family == f) & (o.algo == "sw_original")]
        b = o[(o.family == f) & (o.algo == "sw_relabelled")]
        ta, tb = a.seconds.median(), b.seconds.median()
        pa = a.ops.iloc[0]
        pb = b.groupby("instance").ops.first().mean()
        order_rows[f] = {"orig_s": float(ta), "relab_s": float(tb), "orig_picks": float(pa),
                         "relab_picks": float(pb)}
        lines.append(f"| {f} | {fmt_s(ta)} ({fmt_s(a.seconds.quantile(.25))}-{fmt_s(a.seconds.quantile(.75))}) | "
                     f"{int(pa):,} | {fmt_s(tb)} ({fmt_s(b.seconds.quantile(.25))}-{fmt_s(b.seconds.quantile(.75))}) | "
                     f"{pb:,.0f} | {tb / ta:.2f} |")
    picks = np.array([[r["orig_picks"], r["orig_s"]] for r in order_rows.values()] +
                     [[r["relab_picks"], r["relab_s"]] for r in order_rows.values()])
    pick_time_corr = float(np.corrcoef(picks[:, 0], picks[:, 1])[0, 1])
    lines += ["", f"Correlation between pick changes and time across all 12 rows above: {pick_time_corr:.2f}"]

    with open("results/tables.md", "w") as fh:
        fh.write("\n".join(lines) + "\n")

    swn = x[x.n == nmax].sw
    key = {
        "n_max": int(nmax),
        "sw_time_at_nmax_min_s": float(swn.min()),
        "sw_time_at_nmax_max_s": float(swn.max()),
        "sw_scans_vary_m": sorted({int(o) for o in v[v.algo == "stoer_wagner"].ops}),
        "projection_ratio_min": float(min(ratios)),
        "projection_ratio_max": float(max(ratios)),
        "success_ratio_to_bound_min": float(ss.ratio.min()),
        "success_ratio_to_bound_max": float(ss.ratio[np.isfinite(ss.ratio)].max()),
        "sw_order": order_rows,
        "success_model_inside_ci": [int(model_hits), int(model_total)],
        "sw_pick_changes_vs_time_correlation": pick_time_corr,
        "families_where_measured_karger_beats_sw_at_nmax":
            sorted(x[(x.n == nmax) & (x.karger_measured < x.sw)].family.tolist()),
    }
    with open("results/key_numbers.json", "w") as fh:
        json.dump(key, fh, indent=2)
    print(json.dumps(key, indent=2))


if __name__ == "__main__":
    main()
