#!/usr/bin/env python3
"""Generate a Markdown report from KVB JSON produced by parse_log.py."""

from __future__ import annotations

import json
import sys
from pathlib import Path
from typing import Any


def get_runs(data: dict[str, Any]) -> list[dict[str, Any]]:
    if "runs" in data:
        return data.get("runs", [])
    return [{
        "kernel": data.get("kernel", {}),
        "platform": data.get("platform", {}),
        "summary": data.get("summary", {}),
        "tests": data.get("tests", []),
    }]


def print_run(run: dict[str, Any], idx: int) -> None:
    kernel = run.get("kernel", {}) or {}
    summary = run.get("summary", {}) or {}
    tests = run.get("tests", []) or []
    platform = run.get("platform", {}) or {}

    name = kernel.get("name") or "unknown"
    version = kernel.get("version") or ""

    print(f"## Run {idx}: {name}")
    if version:
        print(f"Kernel version: `{version}`")
    if platform:
        board = platform.get("board", "")
        cpu = platform.get("cpu", "")
        if board or cpu:
            print(f"Platform: `{board}` `{cpu}`")
    print()

    print("### Summary")
    print()
    print("| Pass | Fail | Unsupported | Skipped | Invalid | Total |")
    print("|---:|---:|---:|---:|---:|---:|")
    print("| {pass} | {fail} | {unsupported} | {skipped} | {invalid} | {total} |".format(
        pass=summary.get("pass", 0),
        fail=summary.get("fail", 0),
        unsupported=summary.get("unsupported", 0),
        skipped=summary.get("skipped", 0),
        invalid=summary.get("invalid", 0),
        total=summary.get("total", len(tests)),
    ))

    print()
    print("### Tests")
    print()
    print("| Test ID | Group | State | Message |")
    print("|---|---|---|---|")
    for test in tests:
        print("| {test_id} | {group} | {state} | {message} |".format(
            test_id=test.get("test_id", ""),
            group=test.get("group", ""),
            state=test.get("state", ""),
            message=str(test.get("message", "")).replace("|", "\\|"),
        ))

    print()
    print("### Metrics")
    for test in tests:
        metrics = test.get("metrics", {}) or {}
        if not metrics:
            continue
        print()
        print(f"#### {test.get('test_id', '')}")
        print()
        print("| Metric | Value | Unit |")
        print("|---|---:|---|")
        for metric_name, metric in metrics.items():
            print("| {} | {} | {} |".format(metric_name, metric.get("value", ""), metric.get("unit", "")))
    print()


def main() -> int:
    if len(sys.argv) != 2:
        print("usage: generate_report.py <kvb-results.json>", file=sys.stderr)
        return 2

    data = json.loads(Path(sys.argv[1]).read_text())
    runs = get_runs(data)

    print("# KVB Report")
    print()
    print(f"Complete runs parsed: **{len(runs)}**")
    print()

    for i, run in enumerate(runs, 1):
        print_run(run, i)

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
