---
name: aap-troubleshoot
description: >
  Troubleshoot OpenShift bare-metal install and cluster problems. Use when: a job
  fails on aap.example.com or aap-test.example.com; a user pastes a job URL or job ID;
  debugging phase_1, phase_3, phase_4, phase_5, or auto-gitops; investigating an
  OpenShift bare-metal install failure; workers not joining; nodes not ready;
  checking stdout, host summaries, or failed events from AAP. Starts with OCP-level
  checks (nodes, CSRs, Machines, BareMetalHosts) before pulling AAP job data.
  Keywords: AAP, job failed, install failed, phase 3, phase 4, NTO, iDRAC, bastion,
  unreachable, stdout, job ID, aap.example.com, workers not joining, node not ready,
  CSR pending.
license: Apache-2.0
compatibility: >
  Requires network access to your AAP instance, plus git.
  Helper scripts live in scripts/. Run scripts/aap-setup to initialise credentials.
metadata:
  author: platform-team
  version: "2.2"
allowed-tools: Bash Read
---

# AAP Troubleshooting

## Step 0 — Triage the problem domain (do this first)

Before pulling any AAP data, classify what kind of problem this is:

| User input | Domain | Start with |
|---|---|---|
| Provides a job ID or URL | AAP | Step 1 (extract job ID) |
| Describes an automation / playbook failure | AAP | Step 1 |
| Describes a cluster / workload symptom ("workers didn't join", "node not ready") | OCP-first | OCP checks below |
| Unclear / ambiguous | OCP-first | OCP checks below, escalate to AAP if needed |

### OCP-level checks (for cluster/workload symptoms)

Run these **before** opening AAP. Many problems are diagnosable entirely at the OCP layer:

```bash
# Are the nodes visible at all?
oc get nodes -o wide

# Pending CSRs? (worker nodes need two approval rounds: bootstrap + client)
oc get csr | grep Pending

# Machine object status (is OCP even trying to provision these nodes?)
oc get machines -n openshift-machine-api -o wide

# BareMetalHost state (did hardware provisioning complete?)
oc get bmh -n openshift-machine-api

# Events on stuck objects
oc describe machine -n openshift-machine-api <name>
oc describe node <name>

# If node is reachable — kubelet and network logs on the RHCOS node.
# Agent constraint: never SSH to nodes directly without explicit authorization.
# Ask the user to run these and paste the output back:
#   ssh core@<node-ip> 'journalctl -u kubelet --no-pager | tail -50'
#   ssh core@<node-ip> 'journalctl -u NetworkManager --no-pager | tail -30'
```

**Escalate to AAP (Steps 1–8) when:**
- Cluster state alone doesn't explain the failure
- You need to understand what the automation did (which tasks ran, what output was produced)
- The user specifically asks to review the job

---

## Prerequisites

Credentials at `~/.config/aap/token` and `~/.config/aap/hostname`.
If missing or empty, stop and tell the user to run `./scripts/aap-setup`.

```bash
# Verify
cat ~/.config/aap/hostname
wc -c < ~/.config/aap/token   # should be > 0
```

## Tooling assumptions (important)

- Prefer `jq` for JSON shaping/parsing in shell pipelines.
- If Python is needed, use `python3` explicitly (never assume `python` exists).
- If a required tool is missing (`jq`, `python3`, `gh`, etc.), stop and ask the
  user to install/provide it before continuing.

## Quick triage — pick your path before fetching anything

Use elapsed time as the first signal to avoid unnecessary stdout fetches:

| Elapsed | Likely category | Start with |
|---|---|---|
| < 1 min | Infrastructure / SCM | Step 6 (failed events) |
| 1–5 min | Config / input bug | Step 6 (failed events) — one query usually gives root cause |
| 5–25 min | Boot / iDRAC issue | Step 6, then Step 5 (stdout scan) if events are ambiguous |
| 25–90 min | Install-time issue (NTO, bootstrap) | Step 5 (stdout scan) |
| > 90 min | Severe timeout (network, Ironic) | Step 5 (stdout scan) |

**For fast failures (< 5 min): go directly to Step 6.** Fetching stdout for a 2-minute job wastes an API call.

---

## Scope guardrail for run-history listings

- For run/history listings, default to the most recent **20** runs.
- Do not fetch more than 20 unless the user explicitly asks for a wider scan.
- `scripts/aap-runs.py` now enforces a hard cap of 20; if the user asks for more,
  use direct paginated API queries and state the expanded scope explicitly.

---

## Step 1 — Extract the Job ID

From a URL like `https://aap.example.com/#/jobs/playbook/328968/output`, the job ID is `328968`.

## Step 2 — Fetch Job Metadata

```bash
~/.config/aap/aap-api GET /api/v2/jobs/<id>/
```

Key fields to note:
- `status` — `failed`, `successful`, `canceled`, `running`
- `name` / `playbook` — which template and playbook ran
- `host_status_counts` — breakdown of changed/failed/ok hosts
- `started` / `finished` / `elapsed` — timing
- `source_workflow_job.id` — parent workflow ID if triggered from a workflow
- `extra_vars` — the input variables (cluster name, IPs, etc.)
- `scm_branch` — code version that ran

## Step 3 — Compare Job Version vs Remote Repository

From the job metadata, capture:
- `scm_branch` — the branch/tag AAP checked out (e.g. `v2.5.0`)
- `scm_revision` — the exact commit SHA that ran

**Always fetch before comparing** — a stale local checkout will silently miss fixes that exist on remote branches:

```bash
git fetch --all --tags
```

Then check the local state for reference:

```bash
git describe --tags          # e.g. v1.2.0-10-g1a2b3c4
git branch --show-current    # e.g. develop or main
```

Check whether the failing task file has any commits between the job's SHA and **all remote refs**:

```bash
# Replace <sha> with scm_revision from the job, <task_file> with path from event_data.task_path
git log --oneline <sha>..origin/HEAD -- <task_file>

# If the default remote branch is not the right one, check develop or all refs:
git log --oneline <sha>..origin/develop -- <task_file>
git log --all --oneline -- <task_file> | head -10   # all branches + tags
```

If commits appear, diff the task to see what changed:

```bash
git diff <sha>..origin/develop -- <task_file>
```

To see which branches/tags already contain a specific fix commit:

```bash
git branch -r --contains <fix-sha>
git tag --contains <fix-sha>
```

**Always report:** what version ran (branch + SHA), what the latest remote has (tag + SHA), which remote branch contains the fix, and whether the job template's SCM branch needs updating.

If the user asks for merge-readiness or verification gate status (not just a
single failure root cause), activate the **`aap-verification-gate`** skill.

## Step 4 — Scan Stdout (long failures only — skip for < 5 min jobs)

```bash
scripts/aap-scan-stdout.py <id>
# With surrounding context:
scripts/aap-scan-stdout.py <id> --context 3
# Force re-fetch if cached copy is stale:
scripts/aap-scan-stdout.py <id> --no-cache
```

Output is cached automatically to `~/.cache/aap/<id>-stdout.txt`.
The script scans for: NTO, BootProgress, SSH-readiness, install-rc, ISO/iDRAC,
PowerState, Retry, Timeout — no HTML stripping needed (`?format=txt`).

## Step 5 — Check Host Summaries

```bash
~/.config/aap/aap-api GET /api/v2/jobs/<id>/job_host_summaries/
```

Shows per-host counts: `ok`, `changed`, `failures`, `unreachable`, `skipped`.
Unreachable = connectivity problem. Failures = task errors.

## Step 6 — Fetch Failed Events (targeted — best first step for fast failures)

```bash
scripts/aap-failures.py <id>
```

Prints job header (status, cluster, branch, sha, elapsed, host counts, parent workflow)
followed by all failed task events with task name, host, rc, msg, and stderr.

## Step 7 — Check Parent Workflow (if applicable)

If `aap-failures.py` output shows `Workflow: <id>`, walk the full workflow tree:

```bash
scripts/aap-workflow.py <workflow_id>
```

Shows all phases that ran, their status, elapsed time, and which ones failed.
Steer next investigation with `scripts/aap-failures.py` on the failed child job IDs.

## Step 8 — Assess phase_5 cancel/rerun behavior (when phase_5 involved)

When troubleshooting `phase_5.yml` jobs, always evaluate cancel/rerun semantics
using:
[references/phase5-idempotency-rubric.md](references/phase5-idempotency-rubric.md)

Required report line:
- `cancel/rerun expectation: <reprovision|expand-only|no-op|unsafe-ambiguous> — <one line reason>`

## Common Failure Patterns

If the error isn't immediately obvious, read:
[references/failure-patterns.md](references/failure-patterns.md)

Patterns are grouped by elapsed time (fast / boot / install-time / severe / infra).
Read it when `aap-failures.py` surfaces an unfamiliar error.

## AAP Instances

| Environment | Hostname |
|---|---|
| Production | `aap.example.com` |
| Test | `aap-test.example.com` |

Switch instances by editing `~/.config/aap/hostname`.

---

## Helper scripts (all in `scripts/`)

| Script | Purpose |
|---|---|
| `aap-failures.py <id>` | Job header + failed + unreachable events; auto-cached |
| `aap-scan-stdout.py <id>` | Stdout signal scan — for long (> 5 min) failures; auto-cached |
| `aap-poll.sh <id> [interval]` | Poll a running job until completion (default 30s); exits 0/1 |
| `aap-workflow.py <wf-id>` | Walk a workflow job tree, show all phase statuses |
| `aap-runs.py <template> [branch]` | List recent runs (default 20, hard cap 20); only widen scope if user asks |
| `aap-api GET <path>` | Raw API access for anything not covered above |

Cached outputs live in `~/.cache/aap/`. Pass `--no-cache` to any script to force refresh.

For reviewing a batch of runs across a branch, use the **`aap-branch-review`** skill.

## Security

`aap-failures.py` and `aap-scan-stdout.py` automatically scan fetched content for
secrets using **detect-secrets** (keyword patterns, private keys, AWS keys, JWTs,
basic-auth URLs) plus a supplement for Ansible `key=value` style credentials.

If a `⚠ SECURITY WARNINGS` block appears in script output:
- Do **not** paste that output into chat, tickets, or shared channels
- Inspect the flagged lines directly in the cached file: `~/.cache/aap/<id>-*.txt`
- If a real secret was logged, treat it as a credential leak: rotate the secret and
  notify the security team

Requires `detect-secrets` installed: `pip install -r scripts/requirements.txt`
