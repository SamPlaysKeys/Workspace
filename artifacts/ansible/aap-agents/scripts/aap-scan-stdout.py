#!/usr/bin/env python3
"""Fetch and scan AAP job stdout for known failure/status signals.

Retrieves stdout as plain text (?format=txt) — no HTML stripping required.
Saves to ~/.cache/aap/<job-id>-stdout.txt automatically (or reuses cache).

Usage:
    aap-scan-stdout.py <job-id> [options]

Options:
    --no-cache      Re-fetch from AAP even if cached copy exists
    --raw           Dump all stdout instead of scanning
    --context <n>   Print n lines of context around each match (default: 0)

Examples:
    aap-scan-stdout.py 328968
    aap-scan-stdout.py 328968 --context 3
    aap-scan-stdout.py 328968 --raw | grep -n "NTO"

Credentials: ~/.config/aap/token and ~/.config/aap/hostname
Run scripts/aap-setup if missing.
"""

import re
import sys
import os
sys.path.insert(0, os.path.dirname(__file__))
from _aap_lib import get_credentials, cached_fetch_text, cache_path, security_scan

SIGNALS = [
    ("FATAL / FAILED",      re.compile(r"fatal:|FAILED!|failed!|UNREACHABLE!", re.I)),
    ("RETRY",               re.compile(r"RETRYING.*attempt", re.I)),
    ("TIMEOUT",             re.compile(r"timed out|timeout|exceeded.*wait", re.I)),
    ("SSH / CONNECTION",    re.compile(r"Failed to connect|SSH Error|Permission denied \(publickey", re.I)),
    ("SYNTAX / UNDEFINED",  re.compile(r"undefined variable|template error|syntax error|error in task", re.I)),
    ("INFRA / ORCHESTRATE", re.compile(r"\bNTO\b|BootProgress|OSRunning|\brc=[0-9]+\b|mount_iso|virtual.?media|\bidrac\b", re.I)),
]


def print_with_context(lines, matches, context_n):
    if context_n == 0:
        for lineno, line in matches:
            print(f"  {lineno:>6}: {line.rstrip()[:140]}")
        return
    to_print = set()
    for lineno, _ in matches:
        idx = lineno - 1
        for i in range(max(0, idx - context_n), min(len(lines), idx + context_n + 1)):
            to_print.add(i)
    match_indices = {lineno - 1 for lineno, _ in matches}
    prev = None
    for idx in sorted(to_print):
        if prev is not None and idx > prev + 1:
            print("  ...")
        marker = ">>>" if idx in match_indices else "   "
        print(f"  {marker} {idx + 1:>6}: {lines[idx].rstrip()[:140]}")
        prev = idx


def main():
    args = sys.argv[1:]
    if not args or args[0] in ("-h", "--help"):
        print(__doc__)
        sys.exit(0)

    job_id        = args[0]
    force_refresh = "--no-cache" in args
    raw_mode       = "--raw" in args
    context_n      = 0
    custom_pattern = None

    i = 1
    while i < len(args):
        if args[i] == "--context" and i + 1 < len(args):
            context_n = int(args[i + 1])
            i += 2
        elif args[i] == "--pattern" and i + 1 < len(args):
            custom_pattern = args[i + 1]
            i += 2
        else:
            i += 1

    token, host = get_credentials()

    text, from_cache = cached_fetch_text(
        f"/api/v2/jobs/{job_id}/stdout/?format=txt",
        token, host,
        job_id=job_id, kind="stdout", force_refresh=force_refresh,
    )
    cached = cache_path(job_id, "stdout", "txt")
    src = f"cache: {cached}" if from_cache else f"fetched → {cached}"
    print(f"Job {job_id} stdout ({src})", file=sys.stderr)

    if raw_mode:
        print(text)
        return

    lines = text.splitlines()
    print(f"Job {job_id} — {len(lines)} lines\n")

    any_matches = False
    signals_to_scan = list(SIGNALS)
    if custom_pattern:
        try:
            signals_to_scan.insert(0, (f"CUSTOM: {custom_pattern}", re.compile(custom_pattern, re.I)))
        except re.error as e:
            print(f"ERROR: Invalid regex pattern '{custom_pattern}': {e}", file=sys.stderr)
            sys.exit(1)

    for label, pattern in signals_to_scan:
        matches = [(i + 1, line) for i, line in enumerate(lines) if pattern.search(line)]
        if not matches:
            continue
        any_matches = True
        print(f"── {label} ({len(matches)} match{'es' if len(matches) != 1 else ''}) ──")
        print_with_context(lines, matches[:25], context_n)
        if len(matches) > 25:
            print(f"  ... and {len(matches) - 25} more")
        print()

    if not any_matches:
        print("No known signal patterns matched.")
        print(f"Try: grep -n 'pattern' {cached}")

    # Security scan — always runs, independent of signal results
    try:
        sec_hits = security_scan(text)
    except ImportError as e:
        print(f"⚠  Security scan skipped: {e}\n")
        sec_hits = []

    if sec_hits:
        unique_lines = {}
        for lineno, stype, line in sec_hits:
            unique_lines.setdefault(lineno, []).append(stype)
        print(f"── ⚠  SECURITY WARNINGS ({len(unique_lines)} line{'s' if len(unique_lines) != 1 else ''} may contain secrets) ──")
        print("   Review carefully before sharing this output with others.\n")
        for lineno in sorted(unique_lines)[:30]:
            types = ", ".join(unique_lines[lineno])
            print(f"  {lineno:>6}: [{types}]")
        if len(unique_lines) > 30:
            print(f"  ... and {len(unique_lines) - 30} more lines")
        print(f"\n  Full stdout: {cached}")


if __name__ == "__main__":
    main()
