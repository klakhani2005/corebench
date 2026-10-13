"""Summarize CoreBench CSV output and compute thread-scaling speedups.

Usage:
    python3 scripts/analyze.py results.csv            # print tables
    python3 scripts/analyze.py results.csv --plot     # also save scaling.png (needs matplotlib)
"""
import argparse
import csv
import statistics
import sys
from collections import defaultdict


def load(path):
    groups = defaultdict(list)  # (workload, category, threads) -> [elapsed_ms]
    with open(path, newline="", encoding="utf-8") as f:
        reader = csv.DictReader(f)
        required = {"workload", "category", "threads", "elapsed_ms"}
        if not reader.fieldnames or not required.issubset(reader.fieldnames):
            sys.exit(f"{path} is missing columns {sorted(required)}; was it produced by ./build/corebench?")
        for row in reader:
            key = (row["workload"], row["category"], int(row["threads"]))
            groups[key].append(float(row["elapsed_ms"]))
    if not groups:
        sys.exit(f"{path} has no data rows")
    return groups


def summarize(groups):
    rows = []
    for (workload, category, threads), values in sorted(groups.items()):
        stdev = statistics.stdev(values) if len(values) > 1 else 0.0
        median = statistics.median(values)
        rows.append({
            "workload": workload, "category": category, "threads": threads,
            "runs": len(values), "median_ms": median, "mean_ms": statistics.mean(values),
            "min_ms": min(values), "max_ms": max(values),
            "cv_pct": 100 * stdev / statistics.mean(values) if statistics.mean(values) else 0.0,
        })
    # Speedup relative to the 1-thread median of the same workload.
    baseline = {r["workload"]: r["median_ms"] for r in rows if r["threads"] == 1}
    for r in rows:
        base = baseline.get(r["workload"])
        r["speedup"] = base / r["median_ms"] if base and r["median_ms"] > 0 else None
    return rows


def print_table(rows):
    print(f"{'workload':<18}{'category':<9}{'threads':>8}{'runs':>6}{'median_ms':>12}{'cv%':>7}{'speedup':>9}")
    for r in rows:
        sp = f"{r['speedup']:.2f}x" if r["speedup"] is not None else "-"
        print(f"{r['workload']:<18}{r['category']:<9}{r['threads']:>8}{r['runs']:>6}"
              f"{r['median_ms']:>12.4f}{r['cv_pct']:>7.1f}{sp:>9}")

    seq = next((r for r in rows if r["workload"] == "sequential_access"), None)
    rnd = next((r for r in rows if r["workload"] == "random_access"), None)
    if seq and rnd and seq["median_ms"] > 0:
        print(f"\nRandom vs sequential access: {rnd['median_ms'] / seq['median_ms']:.2f}x slower "
              "(cost of cache misses)")


def plot(rows, out="scaling.png"):
    try:
        import matplotlib
        matplotlib.use("Agg")
        import matplotlib.pyplot as plt
    except ImportError:
        sys.exit("--plot needs matplotlib: pip install matplotlib")
    by_workload = defaultdict(list)
    for r in rows:
        if r["speedup"] is not None and r["category"] != "cache" and r["workload"] != "std_sort":
            by_workload[r["workload"]].append((r["threads"], r["speedup"]))
    fig, ax = plt.subplots(figsize=(7, 4.5))
    max_t = 1
    for name, pts in sorted(by_workload.items()):
        pts.sort()
        xs, ys = zip(*pts)
        max_t = max(max_t, max(xs))
        ax.plot(xs, ys, marker="o", label=name)
    ax.plot([1, max_t], [1, max_t], linestyle="--", color="gray", label="ideal (linear)")
    ax.set_xlabel("threads")
    ax.set_ylabel("speedup vs 1 thread (median)")
    ax.set_title("CoreBench thread scaling")
    ax.legend()
    ax.grid(alpha=0.3)
    fig.tight_layout()
    fig.savefig(out, dpi=150)
    print(f"\nSaved {out}")


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("csv")
    parser.add_argument("--plot", action="store_true", help="save a scaling.png chart")
    args = parser.parse_args()
    rows = summarize(load(args.csv))
    print_table(rows)
    if args.plot:
        plot(rows)


if __name__ == "__main__":
    main()
