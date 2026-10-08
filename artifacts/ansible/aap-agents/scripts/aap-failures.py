#!/usr/bin/env python3
"""Show job metadata and failed/unreachable task events for an AAP job.

Usage:
    aap-failures.py <job-id> [--no-cache]

Options:
    --no-cache   Re-fetch from AAP even if a cached copy exists

Output is also saved to ~/.cache/aap/<job-id>-failures.json for follow-up.

Example:
    aap-failures.py 328968

Credentials: ~/.config/aap/token and ~/.config/aap/hostname
Run scripts/aap-setup if missing.
"""

import json
import sys
import os
sys.path.insert(0, os.path.dirname(__file__))
from _aap_lib import (
    get_credentials, aap_get, get_job_meta, parse_cluster,
    STATUS_SYMBOL, cache_path, save_cache, security_scan,
)


def fetch_events(job_id, event_type, token, host):
    return aap_get(
        f"/api/v2/jobs/{job_id}/job_events/?event={event_type}&page_size=20",
        token, host,
    ).get("results", [])


def print_events(results, label):
    if not results:
        return
    print(f"── {label} ({len(results)}) ──\n")
    for e in results:
        ed  = e.get("event_data", {})
        res = ed.get("res", {})
        print(f"  Task:   {ed.get('task', '?')}")
        print(f"  Host:   {ed.get('host', '?')}")
        rc = res.get("rc", "")
        if rc != "":
            print(f"  rc:     {rc}")
        msg    = str(res.get("msg",    "") or "").strip()[:500]
        stderr = str(res.get("stderr", "") or "").strip()[:400]
        stdout = str(res.get("stdout", "") or "").strip()[:300]
        if msg:
            print(f"  msg:    {msg}")
        if stderr:
            print(f"  stderr: {stderr}")
        if stdout and not msg and not stderr:
            print(f"  stdout: {stdout}")
        print()


def main():
    if len(sys.argv) < 2 or sys.argv[1] in ("-h", "--help"):
        print(__doc__)
        sys.exit(0)

    job_id        = sys.argv[1]
    force_refresh = "--no-cache" in sys.argv

    token, host = get_credentials()

    job = get_job_meta(job_id, token, host, force_refresh=force_refresh)
    status   = job.get("status", "?")
    elapsed  = round(job.get("elapsed", 0) / 60, 1)
    branch   = job.get("scm_branch", "?")
    sha      = (job.get("scm_revision", "") or "")[:8] or "?"
    started  = (job.get("started",  "") or "")[:16]
    finished = (job.get("finished", "") or "")[:16]
    playbook = job.get("playbook", "?")
    template = job.get("name", "?")
    workflow = (job.get("summary_fields", {}).get("source_workflow_job") or {}).get("id")
    cluster  = parse_cluster(job)
    host_counts = job.get("host_status_counts") or {}
    sym = STATUS_SYMBOL.get(status, "?")

    print(f"{'─' * 60}")
    print(f"Job:       {job_id}")
    print(f"Template:  {template}")
    print(f"Playbook:  {playbook}")
    print(f"Status:    {sym} {status}  ({elapsed}m)")
    print(f"Cluster:   {cluster}")
    print(f"Branch:    {branch}  sha={sha}")
    print(f"Started:   {started}  Finished: {finished}")
    print(f"Hosts:     {host_counts}")
    if workflow:
        print(f"Workflow:  {workflow}  → run: scripts/aap-workflow.py {workflow}")
    print(f"{'─' * 60}\n")

    failed_events      = fetch_events(job_id, "runner_on_failed",      token, host)
    unreachable_events = fetch_events(job_id, "runner_on_unreachable",  token, host)

    if not failed_events and not unreachable_events:
        print("No failed or unreachable events found.")
        print("Job may have failed due to timeout or async failure — try: scripts/aap-scan-stdout.py", job_id)
        return

    all_events = {"failed": failed_events, "unreachable": unreachable_events}
    saved = save_cache(job_id, "failures", all_events)
    print(f"(Cached to {saved})\n")

    print_events(failed_events,      "Failed tasks")
    print_events(unreachable_events, "Unreachable hosts")

    # Security scan on event data
    try:
        sec_hits = security_scan(json.dumps(all_events, indent=2))
    except ImportError as e:
        print(f"⚠  Security scan skipped: {e}")
        sec_hits = []

    if sec_hits:
        unique_lines = {}
        for lineno, stype, line in sec_hits:
            unique_lines.setdefault(lineno, []).append(stype)
        print(f"── ⚠  SECURITY WARNINGS ({len(unique_lines)} line{'s' if len(unique_lines) != 1 else ''} in event data may contain secrets) ──")
        print("   Review carefully before sharing this output with others.")


if __name__ == "__main__":
    main()
