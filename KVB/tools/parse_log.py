#!/usr/bin/env python3
"""
Parse KVB UART logs into JSON.

Supports both:
  * one log containing exactly one [KVB] BEGIN RUN ... END RUN block
  * combined UART captures containing multiple kernels and/or repeated runs

The parser is intentionally tolerant of leading garbage and partial lines.  A
new BEGIN RUN before END RUN discards the partial run, which prevents reset or
UART-capture fragments from contaminating comparison data.
"""

from __future__ import annotations

import json
import re
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


def parse_int_pairs(text: str) -> dict[str, int]:
    out: dict[str, int] = {}
    for item in text.split():
        if "=" not in item:
            continue
        key, value = item.split("=", 1)
        try:
            out[key] = int(value)
        except ValueError:
            pass
    return out


def parse_string_pairs(text: str) -> dict[str, str]:
    out: dict[str, str] = {}
    for item in text.split():
        if "=" not in item:
            continue
        key, value = item.split("=", 1)
        out[key] = value.strip('"')
    return out


def iter_kvb_records(text: str):
    """Yield complete-looking KVB records from noisy UART text.

    UART logs sometimes contain fragments such as:
        [KVB] VERSION 0.1.0-pr.[KVB] BEGIN RUN
    A normal line parser would miss the second record.  This scanner splits at
    every '[KVB] ' token and yields each record prefix independently.
    """
    positions = [m.start() for m in KVB_TOKEN_RE.finditer(text)]
    for idx, start in enumerate(positions):
        end = positions[idx + 1] if idx + 1 < len(positions) else len(text)
        record = text[start:end].strip()
        if not record:
            continue
        # Keep only the first physical line inside this record.  This prevents
        # a truncated record from swallowing the next unrelated console line.
        record = record.splitlines()[0].strip()
        yield record


def new_run() -> dict[str, Any]:
    return {
        "version": "",
        "kernel": {"name": "", "version": ""},
        "features": {},
        "platform": {},
        "summary": {},
        "tests": {},
        "complete": False,
    }


def parse_log(path: Path) -> dict[str, Any]:
    runs: list[dict[str, Any]] = []
    current: dict[str, Any] | None = None

    text = path.read_text(errors="replace")
    for line in iter_kvb_records(text):
        if BEGIN_RUN_RE.match(line):
            # A new BEGIN before END means the prior block was partial/noisy.
            current = new_run()
            continue

        if current is None:
            # Ignore boot banners and orphan fragments outside a run.
            continue

        if END_RUN_RE.match(line):
            current["complete"] = True
            if current["tests"]:
                run = dict(current)
                run["tests"] = list(current["tests"].values())
                runs.append(run)
            current = None
            continue

        m = VERSION_RE.match(line)
        if m:
            current["version"] = m.group(1)
            continue

        m = KERNEL_RE.match(line)
        if m:
            current["kernel"] = {"name": m.group(1), "version": m.group(2)}
            continue

        m = FEATURES_RE.match(line)
        if m:
            current["features"] = parse_int_pairs(m.group(1))
            continue

        m = PLATFORM_RE.match(line)
        if m:
            current["platform"] = parse_string_pairs(m.group(1))
            current["platform_raw"] = m.group(1)
            continue

        m = TEST_BEGIN_RE.match(line)
        if m:
            test_id, group, name = m.groups()
            current["tests"].setdefault(test_id, {
                "test_id": test_id,
                "group": group,
                "name": name,
                "state": "",
                "message": "",
                "metrics": {},
            })
            continue

        m = RESULT_RE.match(line)
        if m:
            test_id, group, state, message = m.groups()
            test = current["tests"].setdefault(test_id, {
                "test_id": test_id,
                "group": group,
                "name": "",
                "state": "",
                "message": "",
                "metrics": {},
            })
            test["group"] = group
            test["state"] = state
            test["message"] = message
            continue

        m = METRIC_RE.match(line)
        if m:
            test_id, name, value, unit = m.groups()
            test = current["tests"].setdefault(test_id, {
                "test_id": test_id,
                "group": "",
                "name": "",
                "state": "",
                "message": "",
                "metrics": {},
            })
            test["metrics"][name] = {"value": int(value), "unit": unit}
            continue

        m = SUMMARY_RE.match(line)
        if m:
            current["summary"] = parse_int_pairs(m.group(1))
            continue

    out: dict[str, Any] = {"run_count": len(runs), "runs": runs}

    # Backward compatibility with the original one-run JSON schema.
    if len(runs) == 1:
        out["summary"] = runs[0].get("summary", {})
        out["tests"] = runs[0].get("tests", [])
        out["kernel"] = runs[0].get("kernel", {})
        out["platform"] = runs[0].get("platform", {})

    return out


def main() -> int:
    if len(sys.argv) != 2:
        print("usage: parse_log.py <kvb.log>", file=sys.stderr)
        return 2

    data = parse_log(Path(sys.argv[1]))
    json.dump(data, sys.stdout, indent=2)
    print()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
