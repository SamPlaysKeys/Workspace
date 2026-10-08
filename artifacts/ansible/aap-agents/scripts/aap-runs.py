#!/usr/bin/env python3
"""List recent AAP job runs for a given job template, optionally filtered by branch.

Usage:
    aap-runs.py <template-name> [branch] [--limit N]

Options:
    --limit N   Maximum number of results to fetch (default: 20, hard cap: 20)

Examples:
    aap-runs.py "Install CoreOS 2.0"
    aap-runs.py "Deploy Openshift 2.0" develop
    aap-runs.py "Validate 2.0" feature/network-refactor --limit 20

Output columns: ID  STATUS  ELAPSED  CLUSTER  BRANCH  SHA  STARTED

Credentials: ~/.config/aap/token and ~/.config/aap/hostname
Run scripts/aap-setup if missing.
"""

import sys
import urllib.parse

# Allow running from repo root without installing
import os
sys.path.insert(0, os.path.dirname(__file__))
from _aap_lib import get_credentials, aap_get, parse_cluster, STATUS_SYMBOL

DEFAULT_LIMIT = 20
HARD_CAP_LIMIT = 20


def main():
    if len(sys.argv) < 2 or sys.argv[1] in ("-h", "--help"):
        print(__doc__)
        sys.exit(0)

    template = sys.argv[1]
    branch_filter = None
    limit = DEFAULT_LIMIT

    i = 2
    while i < len(sys.argv):
        if sys.argv[i] == "--limit" and i + 1 < len(sys.argv):
            try:
                requested_limit = int(sys.argv[i + 1])
            except ValueError:
                print(f"Invalid --limit value '{sys.argv[i + 1]}'. Expected an integer.", file=sys.stderr)
                sys.exit(2)
            if requested_limit > HARD_CAP_LIMIT:
                print(
                    f"Requested --limit {requested_limit} exceeds hard cap {HARD_CAP_LIMIT}; "
                    f"using {HARD_CAP_LIMIT}.",
                    file=sys.stderr,
                )
            limit = min(requested_limit, HARD_CAP_LIMIT)
            i += 2
        elif sys.argv[i] == "--limit":
            print("Missing value for --limit.", file=sys.stderr)
            sys.exit(2)
        elif not sys.argv[i].startswith("--"):
            branch_filter = sys.argv[i]
            i += 1
        else:
            i += 1

    token, host = get_credentials()

    params = urllib.parse.urlencode({"order_by": "-id", "page_size": limit, "name": template})
    data = aap_get(f"/api/v2/jobs/?{params}", token, host)

    results = data.get("results", [])
    if branch_filter:
        # Exact match — substring match causes false positives (e.g. "develop" matches "feature/develop-foo")
        results = [j for j in results if j.get("scm_branch", "") == branch_filter]

    if not results:
        msg = f"No runs found for template '{template}'"
        if branch_filter:
            msg += f" on branch '{branch_filter}'"
        print(msg)
        return

    print(f"{'ID':<10} {'STATUS':<14} {'ELAPSED':>8}  {'CLUSTER':<22} {'BRANCH':<35} {'SHA':<9} STARTED")
    print("-" * 115)
    for j in results:
        status  = j.get("status", "?")
        elapsed = round(j.get("elapsed", 0) / 60, 1)
        branch  = j.get("scm_branch", "?")
        sha     = (j.get("scm_revision", "") or "")[:8] or "?"
        started = (j.get("started", "") or "")[:16]
        cluster = parse_cluster(j)
        sym     = STATUS_SYMBOL.get(status, "?")
        print(f"{j['id']:<10} {sym} {status:<12} {elapsed:>6.1f}m  {cluster:<22} {branch:<35} {sha:<9} {started}")


if __name__ == "__main__":
    main()
