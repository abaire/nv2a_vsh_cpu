#!/usr/bin/env python3
"""Extracts expectations.json from an nxdk_low_level_nv2a_tests hardware log.

The input is the vsh_log.json written by the VSH test XBE from
nxdk_low_level_nv2a_tests when it runs on a real Xbox. Every result event of
the "VSH Exceptional *" tests carries its operands and the measured result, so
this script only reformats the log; it does not read any of that project's source code. Result events are
identified by their "record_op" field, and the git revision of the test build
is read from the log's top-level "git_revision" and "git_dirty" keys. Logs from
builds with uncommitted changes are rejected unless --allow-dirty is given.

The measured result ("actual") becomes the expected "hw" value, regardless of
whether the test passed or failed against its own tables.

Usage: extract_expectations.py vsh_log.json [expectations.json]

The output path defaults to expectations.json next to this script. See
"Updating the exceptional value expectations" in README.md for the full process.
"""

from __future__ import annotations

import argparse
import json
from pathlib import Path
from typing import Any

_HERE = Path(__file__).resolve().parent
_TEST_PREFIX = "VSH Exceptional "
_OPERANDS = (("in_a", "a"), ("in_b", "b"), ("in_c", "c"), ("in_P", "P"))
_REQUIRED_FIELDS = ("group", "case")
_ARL_OUT_OF_RANGE = "out of range (reads 0)"

Event = dict[str, Any]
Record = dict[str, Any]


class LogFormatError(Exception):
    """The log does not have the structure this script expects."""


def _operand(value: str) -> str | list[str]:
    bits = [f"0x{int(b, 16):08X}" for b in value.split(",")]
    return bits if len(bits) > 1 else bits[0]


def _hw(fields: dict[str, Any]) -> str:
    if fields["record_op"] != "ARL":
        return f"0x{int(fields['actual'], 16):08X}"
    if fields["kind"] == "zero":
        return _ARL_OUT_OF_RANGE
    return f"c[{fields['decoded_index']}]"


def _record(fields: dict[str, Any]) -> Record:
    record: Record = {
        "group": fields["group"],
        "op": fields["record_op"],
        "case": fields["case"],
        "inputs": {name: _operand(fields[key]) for key, name in _OPERANDS if key in fields},
    }
    if "component" in fields:
        record["component"] = fields["component"]
    record["hw"] = _hw(fields)
    return record


def _exceptional_tests(log: dict[str, Any]) -> list[dict[str, Any]]:
    tests = [t for t in log["tests"] if t["name"].startswith(_TEST_PREFIX)]
    if not tests:
        msg = f'No "{_TEST_PREFIX}*" tests in the log'
        raise LogFormatError(msg)
    return tests


def _source(log: dict[str, Any], *, allow_dirty: bool) -> dict[str, Any]:
    revision = log.get("git_revision")
    dirty = log.get("git_dirty")
    if not isinstance(revision, str) or not isinstance(dirty, bool):
        msg = "The log does not record the git revision of the test build"
        raise LogFormatError(msg)
    if not allow_dirty and (dirty or revision == "unknown"):
        msg = f"The test build ({revision}) had uncommitted changes or an unknown revision"
        raise LogFormatError(msg)
    return {"git_revision": revision, "git_dirty": dirty}


def extract(log: dict[str, Any], *, allow_dirty: bool = False) -> dict[str, Any]:
    records: list[Record] = []
    seen: set[tuple[str, str, str, str | None]] = set()
    for test in _exceptional_tests(log):
        for event in test["events"]:
            fields: dict[str, Any] = event.get("fields", {})
            # Result events are the ones with a record_op; others, such as the
            # checks that the attribute and constant paths agree, are skipped.
            if "record_op" not in fields:
                continue
            missing = [f for f in _REQUIRED_FIELDS if f not in fields]
            if missing:
                msg = f"{test['name']}: event is missing {missing}: {event['message']}"
                raise LogFormatError(msg)
            record = _record(fields)
            key = (record["group"], record["op"], record["case"], record.get("component"))
            if key in seen:
                msg = f"{test['name']}: duplicate record {key}"
                raise LogFormatError(msg)
            seen.add(key)
            records.append(record)
    if not records:
        msg = "The log has no exceptional value records"
        raise LogFormatError(msg)
    return {"source": _source(log, allow_dirty=allow_dirty), "records": records}


def _dump(data: dict[str, Any]) -> str:
    """Formats `data` with one record per line so that diffs stay readable."""
    records = ",\n".join(json.dumps(r, separators=(",", ":")) for r in data["records"])
    source = json.dumps(data["source"], separators=(",", ":"))
    return f'{{\n"source":{source},\n"records":[\n{records}\n]\n}}\n'


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("log", type=Path, help="vsh_log.json from a hardware run")
    parser.add_argument("dst", nargs="?", type=Path, default=_HERE / "expectations.json")
    parser.add_argument(
        "--allow-dirty",
        action="store_true",
        help="accept a log from a test build with uncommitted changes, for experiments only",
    )
    args = parser.parse_args()

    with args.log.open(encoding="utf-8") as infile:
        log = json.load(infile)
    data = extract(log, allow_dirty=args.allow_dirty)
    with args.dst.open("w", encoding="ascii", newline="\n") as outfile:
        outfile.write(_dump(data))


if __name__ == "__main__":
    main()
