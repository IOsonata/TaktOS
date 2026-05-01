#!/usr/bin/env python3
"""
Aggregate KVB logs into fair per-kernel comparison tables.

This version understands combined UART captures.  If a single input file
contains FreeRTOS, TaktOS, ThreadX, and Zephyr runs, the tool groups complete
runs by the [KVB] KERNEL name automatically.

Usage:
    python3 KVB/tools/compare_runs.py combined_uart.log

    python3 KVB/tools/compare_runs.py --auto-kernel combined_uart.log

    python3 KVB/tools/compare_runs.py \
        --label TaktOS taktos1.log taktos2.log \
        --label Zephyr zephyr1.log zephyr2.log
"""

from __future__ import annotations

import argparse
import math
import re
import statistics
import sys
from pathlib import Path
from typing import Any

KVB_TOKEN_RE = re.compile(r"\[KVB\] ")
BEGIN_RUN_RE = re.compile(r"^\[KVB\] BEGIN RUN$")
END_RUN_RE = re.compile(r"^\[KVB\] END RUN$")
VERSION_RE = re.compile(r"^\[KVB\] VERSION (.*)$")
KERNEL_RE = re.compile(r"^\[KVB\] KERNEL name=(\S+) version=(.*)$")
FEATURES_RE = re.compile(r"^\[KVB\] FEATURES (.*)$")
PLATFORM_RE = re.compile(r"^\[KVB\] PLATFORM (.*)$")
TEST_BEGIN_RE = re.compile(r'^\[KVB\] BEGIN id=(\S+) group=(\S+) name="(.*)"$')
RESULT_RE = re.compile(r'^\[KVB\] RESULT id=(\S+) group=(\S+) state=(\S+) message="(.*)"$')
METRIC_RE = re.compile(r"^\[KVB\] METRIC id=(\S+) name=(\S+) value=(-?\d+) unit=(.*)$")
SUMMARY_RE = re.compile(r"^\[KVB\] SUMMARY (.*)$")

# Higher is better except latency/duration metrics.
METRIC_DIRECTION: dict[tuple[str, str], str] = {
    ("SCHED_COOP_001", "total_yields"): "higher",
    ("SYNC_SEM_FAST_001", "throughput"): "higher",
    ("SYNC_MUTEX_FAST_001", "throughput"): "higher",
    ("IPC_QUEUE_FAST_001", "throughput"): "higher",
    ("TIME_SLEEP_001", "elapsed_us"): "lower",
}

HEADLINE_ORDER = [
    ("SCHED_COOP_001", "total_yields"),
    ("SYNC_SEM_FAST_001", "throughput"),
    ("SYNC_MUTEX_FAST_001", "throughput"),
    ("IPC_QUEUE_FAST_001", "throughput"),
    ("TIME_SLEEP_001", "elapsed_us"),
]


def parse_int_pairs(text: str) -> dict[str, int]:
    out: dict[str, int] = {}
    for item in text.split():
        if "=" not in item:
            continue
        k, v = item.split("=", 1)
        try:
            out[k] = int(v)
        except ValueError:
            pass
    return out


def iter_kvb_records(text: str):
    positions = [m.start() for m in KVB_TOKEN_RE.finditer(text)]
    for idx, start in enumerate(positions):
        end = positions[idx + 1] if idx + 1 < len(positions) else len(text)
        record = text[start:end].strip()
        if record:
            yield record.splitlines()[0].strip()


def new_run() -> dict[str, Any]:
    return {
        "kernel_name": "unknown",
        "kernel_version": "",
        "summary": {},
        "tests": {},
    }


def parse_runs(path: Path) -> list[dict[str, Any]]:
    runs: list[dict[str, Any]] = []
    current: dict[str, Any] | None = None

    for line in iter_kvb_records(path.read_text(errors="replace")):
        if BEGIN_RUN_RE.match(line):
            current = new_run()
            continue

        if current is None:
            continue

        if END_RUN_RE.match(line):
            if current["tests"]:
                runs.append(current)
            current = None
            continue

        m = KERNEL_RE.match(line)
        if m:
            current["kernel_name"] = m.group(1)
            current["kernel_version"] = m.group(2)
            continue

        m = SUMMARY_RE.match(line)
        if m:
            current["summary"] = parse_int_pairs(m.group(1))
            continue

        m = TEST_BEGIN_RE.match(line)
        if m:
            tid, group, name = m.groups()
            current["tests"].setdefault(tid, {
                "test_id": tid, "group": group, "name": name,
                "state": "", "metrics": {},
            })
            continue

        m = RESULT_RE.match(line)
        if m:
            tid, group, state, _message = m.groups()
            test = current["tests"].setdefault(tid, {
                "test_id": tid, "group": group, "name": "",
                "state": "", "metrics": {},
            })
            test["group"] = group
            test["state"] = state
            continue

        m = METRIC_RE.match(line)
        if m:
            tid, name, value, _unit = m.groups()
            test = current["tests"].setdefault(tid, {
                "test_id": tid, "group": "", "name": "",
                "state": "", "metrics": {},
            })
            test["metrics"][name] = int(value)
            continue

    return runs


def aggregate(runs: list[dict[str, Any]]) -> dict[str, dict[str, Any]]:
    test_ids: set[str] = set()
    for run in runs:
        test_ids.update(run["tests"].keys())

    out: dict[str, dict[str, Any]] = {}
    for tid in sorted(test_ids):
        states = [run["tests"][tid].get("state", "") for run in runs if tid in run["tests"]]
        if states and all(s == "PASS" for s in states):
            state_summary = "PASS"
        elif states and all(s == "FAIL" for s in states):
            state_summary = "FAIL"
        else:
            n_pass = sum(1 for s in states if s == "PASS")
            state_summary = f"mixed ({n_pass}/{len(states)} PASS)"

        metric_names: set[str] = set()
        for run in runs:
            if tid in run["tests"]:
                metric_names.update(run["tests"][tid].get("metrics", {}).keys())

        metrics: dict[str, dict[str, Any]] = {}
        for name in sorted(metric_names):
            values = [run["tests"][tid]["metrics"][name]
                      for run in runs
                      if tid in run["tests"] and name in run["tests"][tid].get("metrics", {})]
            if not values:
                continue
            mean = statistics.mean(values)
            stdev = statistics.stdev(values) if len(values) > 1 else 0.0
            metrics[name] = {
                "mean": mean,
                "stdev": stdev,
                "min": min(values),
                "max": max(values),
                "n": len(values),
                "rel_stdev_pct": (stdev / mean * 100.0) if mean else 0.0,
            }

        out[tid] = {"state_summary": state_summary, "n_runs": len(states), "metrics": metrics}
    return out


def fmt_value(mean: float, stdev: float, n: int) -> str:
    if n <= 1:
        return f"{mean:,.0f}"
    mean_str = f"{mean:,.0f}" if abs(mean) >= 1000 else f"{mean:.2f}"
    if stdev >= 100:
        stdev_str = f"{stdev:,.0f}"
    elif stdev >= 1:
        stdev_str = f"{stdev:.1f}"
    else:
        stdev_str = f"{stdev:.3f}"
    return f"{mean_str} ± {stdev_str}"


def print_single_label(label: str, agg: dict[str, Any]) -> None:
    print(f"\n## {label}\n")
    print("| Test | State | Metric | Mean ± stddev | Min | Max | rel.σ | N |")
    print("|---|---|---|---:|---:|---:|---:|---:|")
    for tid, info in agg.items():
        for name, m in info["metrics"].items():
            print(f"| {tid} | {info['state_summary']} | {name} | "
                  f"{fmt_value(m['mean'], m['stdev'], m['n'])} | "
                  f"{m['min']:,} | {m['max']:,} | "
                  f"{m['rel_stdev_pct']:.2f}% | {m['n']} |")


def comparison_ratio(a_label: str, a: float, b_label: str, b: float,
                     direction: str) -> str:
    if a == 0 or b == 0 or math.isnan(a) or math.isnan(b):
        return "—"
    if direction == "lower":
        if a <= b:
            return f"{a_label} {b / a:.2f}× lower"
        return f"{b_label} {a / b:.2f}× lower"
    if a >= b:
        return f"{a_label} {a / b:.2f}× higher"
    return f"{b_label} {b / a:.2f}× higher"


def print_comparison(labels_aggs: list[tuple[str, dict[str, Any]]]) -> None:
    if len(labels_aggs) < 2:
        return

    print("\n## Comparison\n")
    headers = ["Test", "Metric", "Better"] + [lbl for lbl, _ in labels_aggs]
    if len(labels_aggs) == 2:
        headers.append("Ratio")
    print("| " + " | ".join(headers) + " |")
    print("|" + "|".join(["---"] * len(headers)) + "|")

    for tid, metric_name in HEADLINE_ORDER:
        direction = METRIC_DIRECTION.get((tid, metric_name), "higher")
        cells: list[str] = []
        means: list[float] = []
        present = True
        for _label, agg in labels_aggs:
            info = agg.get(tid)
            if info is None or metric_name not in info["metrics"]:
                cells.append("—")
                means.append(float("nan"))
                present = False
                continue
            m = info["metrics"][metric_name]
            cells.append(fmt_value(m["mean"], m["stdev"], m["n"]))
            means.append(m["mean"])

        row = [tid, metric_name, "lower" if direction == "lower" else "higher"] + cells
        if len(labels_aggs) == 2 and present:
            row.append(comparison_ratio(labels_aggs[0][0], means[0], labels_aggs[1][0], means[1], direction))
        print("| " + " | ".join(row) + " |")


def build_groups(args: argparse.Namespace) -> list[tuple[str, list[dict[str, Any]]]]:
    if args.label:
        groups: list[tuple[str, list[dict[str, Any]]]] = []
        for grp in args.label:
            if len(grp) < 2:
                raise SystemExit(f"--label needs LABEL plus at least one log: {grp}")
            label, *paths = grp
            runs: list[dict[str, Any]] = []
            for p in paths:
                runs.extend(parse_runs(Path(p)))
            groups.append((label, runs))
        return groups

    by_kernel: dict[str, list[dict[str, Any]]] = {}
    for p in args.logs:
        for run in parse_runs(Path(p)):
            by_kernel.setdefault(run.get("kernel_name") or "unknown", []).append(run)
    return sorted(by_kernel.items())


def main(argv: list[str]) -> int:
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--label", action="append", nargs="+", metavar=("LABEL", "LOG"),
                        help="Group logs under a label. Repeatable.")
    parser.add_argument("--auto-kernel", action="store_true",
                        help="Group complete runs by [KVB] KERNEL name. This is the default without --label.")
    parser.add_argument("logs", nargs="*", help="KVB UART logs")
    args = parser.parse_args(argv)

    if not args.label and not args.logs:
        parser.print_help()
        return 1

    groups = build_groups(args)
    if not groups:
        print("No complete KVB runs found.", file=sys.stderr)
        return 1

    print("# KVB aggregate report\n")
    aggs: list[tuple[str, dict[str, Any]]] = []
    for label, runs in groups:
        print(f"\n### Inputs for `{label}`")
        print(f"- complete runs parsed: {len(runs)}")
        versions = sorted({r.get("kernel_version", "") for r in runs if r.get("kernel_version")})
        if versions:
            print(f"- versions: {', '.join(versions)}")
        agg = aggregate(runs)
        aggs.append((label, agg))
        print_single_label(label, agg)

    print_comparison(aggs)

    print("\n---\n")
    print("Statistics: arithmetic mean, sample standard deviation, min/max across complete runs. ")
    print("Combined UART logs are split by [KVB] BEGIN RUN / [KVB] END RUN and grouped by kernel name unless --label is used.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
