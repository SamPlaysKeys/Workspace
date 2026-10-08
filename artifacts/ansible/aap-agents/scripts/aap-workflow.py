#!/usr/bin/env python3
"""Walk an AAP workflow job and print a tree of its child jobs.

Useful when a job was triggered by a workflow (aap-failures.py shows the workflow ID)
and you want to see all phases that ran, their status, and which ones failed.

Usage:
    aap-workflow.py <workflow-job-id> [--no-cache]

Options:
    --no-cache   Re-fetch from AAP even if a cached copy exists

Example:
    aap-workflow.py 328900
    # Job metadata shows: Workflow: 328900 — run this to see the full picture

Output:
    Workflow summary header, then a tree of all nodes showing:
      job ID | status | elapsed | cluster | template name | playbook

Credentials: ~/.config/aap/token and ~/.config/aap/hostname
Run scripts/aap-setup if missing.
"""

import json
import sys
import os
sys.path.insert(0, os.path.dirname(__file__))
from _aap_lib import (
    get_credentials, aap_get, parse_cluster,
    STATUS_SYMBOL, cached_aap_get,
)


def get_workflow(wf_id, token, host, force_refresh):
    data, _ = cached_aap_get(
        f"/api/v2/workflow_jobs/{wf_id}/",
        token, host,
        job_id=f"wf{wf_id}", kind="meta",
        force_refresh=force_refresh,
    )
    return data


def get_workflow_nodes(wf_id, token, host, force_refresh):
    data, _ = cached_aap_get(
        f"/api/v2/workflow_jobs/{wf_id}/workflow_nodes/?page_size=50",
        token, host,
        job_id=f"wf{wf_id}", kind="nodes",
        force_refresh=force_refresh,
    )
    return data.get("results", [])


def resolve_child_job(node, token, host):
    """Return a flat dict of useful fields for a workflow node's child job (if any)."""
    sf = node.get("summary_fields", {})

    # A node can link to a job, project update, inventory update, or workflow job
    for kind in ("job", "project_update", "inventory_update", "workflow_job"):
        child = sf.get(kind)
        if child:
            job_id  = child.get("id", "?")
            status  = child.get("status", "?")
            name    = child.get("name", child.get("playbook", "?"))
            elapsed = child.get("elapsed", 0)
            # For full job details (cluster, branch) we'd need another fetch;
            # summary_fields gives us enough for the tree view
            return {
                "kind":    kind,
                "id":      job_id,
                "status":  status,
                "name":    name,
                "elapsed": round(elapsed / 60, 1) if elapsed else 0,
            }
    # Node hasn't run yet (waiting/never-triggered)
    return {
        "kind":    "pending",
        "id":      "—",
        "status":  node.get("do_not_run") and "skipped" or "pending",
        "name":    (node.get("summary_fields", {}).get("unified_job_template") or {}).get("name", "?"),
        "elapsed": 0,
    }


def build_tree(nodes):
    """Return (roots, children_map) for rendering a tree."""
    id_map = {n["id"]: n for n in nodes}
    children = {n["id"]: [] for n in nodes}
    roots = []
    for n in nodes:
        parents = [e["id"] for e in (n.get("success_nodes") or []) + (n.get("failure_nodes") or []) + (n.get("always_nodes") or [])]
        # Workflow node links go *forward* (success_nodes are children of this node)
        # We need to find who points TO this node
    # AAP workflow_nodes: each node has success_nodes/failure_nodes/always_nodes
    # listing the IDs of nodes that follow. Build parent→children map.
    child_ids = set()
    for n in nodes:
        for edge_key in ("success_nodes", "failure_nodes", "always_nodes"):
            for child_id in (n.get(edge_key) or []):
                # child_id may be a dict or an int depending on API version
                cid = child_id if isinstance(child_id, int) else child_id.get("id", child_id)
                children[n["id"]].append((edge_key.replace("_nodes", ""), cid))
                child_ids.add(cid)
    roots = [n for n in nodes if n["id"] not in child_ids]
    return roots, children


def print_tree(nodes, roots, children_map, indent=0):
    prefix = "  " * indent
    for node in roots:
        child = resolve_child_job(node, None, None)
        sym   = STATUS_SYMBOL.get(child["status"], "?")
        elapsed_str = f"{child['elapsed']:.1f}m" if child["elapsed"] else "—"
        print(f"{prefix}{'└─' if indent else '  '} [{child['id']}] {sym} {child['status']:<12} {elapsed_str:>6}  {child['name']}")
        # Recurse into children
        child_node_ids = [cid for _, cid in children_map.get(node["id"], [])]
        child_nodes    = [n for n in nodes if n["id"] in child_node_ids]
        if child_nodes:
            print_tree(nodes, child_nodes, children_map, indent + 1)


def main():
    if len(sys.argv) < 2 or sys.argv[1] in ("-h", "--help"):
        print(__doc__)
        sys.exit(0)

    wf_id         = sys.argv[1]
    force_refresh = "--no-cache" in sys.argv

    token, host = get_credentials()

    # Workflow metadata
    wf = get_workflow(wf_id, token, host, force_refresh)
    status   = wf.get("status", "?")
    elapsed  = round(wf.get("elapsed", 0) / 60, 1)
    started  = (wf.get("started",  "") or "")[:16]
    finished = (wf.get("finished", "") or "")[:16]
    name     = wf.get("name", "?")
    sym      = STATUS_SYMBOL.get(status, "?")

    try:
        ev = json.loads(wf.get("extra_vars", "{}") or "{}")
    except (json.JSONDecodeError, TypeError):
        ev = {}
    cluster = ev.get("openshift_cluster_name", "?")

    print(f"{'─' * 65}")
    print(f"Workflow:  {wf_id}  —  {name}")
    print(f"Status:    {sym} {status}  ({elapsed}m)")
    print(f"Cluster:   {cluster}")
    print(f"Started:   {started}  Finished: {finished}")
    print(f"{'─' * 65}\n")

    # Nodes
    nodes = get_workflow_nodes(wf_id, token, host, force_refresh)
    if not nodes:
        print("No workflow nodes found.")
        return

    print(f"Nodes ({len(nodes)} total):\n")
    print(f"  {'JOB ID':<10} {'STATUS':<14} {'ELAPSED':>7}  TEMPLATE")
    print(f"  {'-'*60}")

    # Flat list first — simpler and more reliable than tree for AAP's node graph
    for node in nodes:
        child     = resolve_child_job(node, token, host)
        sym       = STATUS_SYMBOL.get(child["status"], "?")
        elapsed_s = f"{child['elapsed']:.1f}m" if child["elapsed"] else "—"
        print(f"  {str(child['id']):<10} {sym} {child['status']:<12} {elapsed_s:>6}  {child['name']}")

    # Summarise failed nodes
    failed = [resolve_child_job(n, token, host) for n in nodes
              if resolve_child_job(n, token, host)["status"] == "failed"]
    if failed:
        print(f"\nFailed jobs — investigate with: scripts/aap-failures.py <id>")
        for f in failed:
            print(f"  {f['id']}  {f['name']}")


if __name__ == "__main__":
    main()
