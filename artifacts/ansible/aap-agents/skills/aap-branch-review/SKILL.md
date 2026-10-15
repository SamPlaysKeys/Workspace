---
name: aap-branch-review
description: >
  Review a batch of recent AAP job runs across a branch to find patterns.
  Use when: checking branch health before merging; summarising what recent runs
  tell us about code reliability; updating verification evidence after new job runs;
  understanding whether failures are code bugs, input bugs, or infrastructure.
  Keywords: branch review, multiple jobs, run history, what's been tested,
  pre-merge check, test verification, aap runs.
license: Apache-2.0
compatibility: >
  Requires network access to your AAP instance, plus git.
  Helper scripts live in scripts/. Run scripts/aap-setup to initialise credentials.
metadata:
  author: platform-team
  version: "2.1"
allowed-tools: Bash Read
---

# AAP Branch Review

This skill analyses a batch of recent AAP job runs on a branch and produces a
structured health summary. It's the complement to `aap-troubleshoot` (single-job
deep-dive) — this one looks across runs to find patterns.

## Prerequisites

Same as `aap-troubleshoot`: credentials at `~/.config/aap/token` and
`~/.config/aap/hostname`. Run `./scripts/aap-setup` if missing.

For a single-job deep-dive, use the **`aap-troubleshoot`** skill instead.

## Tooling assumptions (important)

- Prefer `jq` for JSON shaping/parsing in shell pipelines.
- If Python is needed, use `python3` explicitly (never assume `python` exists).
- If a required tool is missing (`jq`, `python3`, `gh`, etc.), stop and ask the
  user to install/provide it before continuing.

---

## Step 1 — Identify the template names and branch to review

Ask which AAP job template names and which git branch to review. A template
name is the controller's job template name, not a playbook filename. If the
user hasn't named at least one template, stop and ask before querying AAP.

---

## Step 2 — List recent runs, filtered by branch

```bash
scripts/aap-runs.py "<job template name>" <branch>
```

Omit the branch argument to see all recent runs regardless of branch.
The script uses exact branch matching — no false positives from substrings
(e.g. `develop` won't match `feature/develop-foo`).

Scope guardrail:
- Default to the most recent **20** runs.
- Do not fetch more than 20 unless the user explicitly asks for a wider scan.
- `scripts/aap-runs.py` enforces a hard cap of 20; if the user asks for more,
  use direct paginated API queries and state the expanded scope explicitly.

Collect: job ID, status, elapsed minutes, cluster name, commit SHA.

---

## Step 3 — Triage by failure category

Use elapsed time as the first triage signal before pulling any logs:

| Elapsed | Category | What to look for |
|---|---|---|
| < 1 min | **Infrastructure** | Proxy failure, SCM sync error, execution environment issue |
| 1–5 min | **Config / input bug** | Validation failure, malformed input, missing var, dependency install |
| 5–25 min | **Boot / BMC issue** | Media mount, power-state polling, SSH readiness timeout |
| 25–90 min | **Install-time issue** | Condition stuck, bootstrap timeout, component convergence |
| > 90 min | **Severe timeout** | Provisioning slow, network path down, service never came up |

For **config/input failures** (1–5 min): pull failed events directly — one query
usually gives the full root cause.

For **install-time failures** (> 25 min): fetch stdout and scan for known signals
(stuck conditions, installer return codes, ready signals, SSH readiness).

---

## Step 4 — Pull failed events for fast failures

```bash
scripts/aap-failures.py <id>
```

Prints the job header and all failed task events. For config/input failures
(1–5 min), this is usually sufficient to identify root cause without fetching stdout.

---

## Step 5 — For long failures, scan stdout for known signals

```bash
scripts/aap-scan-stdout.py <id>
# With context lines around each match:
scripts/aap-scan-stdout.py <id> --context 3
# Reuse cached copy (auto-saved on first fetch):
# File is at ~/.cache/aap/<id>-stdout.txt — use grep -n for further digging
scripts/aap-scan-stdout.py <id> --no-cache   # force re-fetch if needed
```

The script fetches stdout as plain text (`?format=txt`) — no HTML stripping needed.
Scans for: FATAL/FAILED, retry, timeout, SSH, syntax errors, and Ansible return codes.

Key signals to interpret:
- A condition or resource stuck progressing — the change is not applying
- Boot or power-state retries — boot sequence issue, or an older revision still polling
- SSH readiness retries — the host is up but the installer or service stalled
- Installer timeout return code — the installer's own timer fired; check whether the service came up anyway
- `ASYNC_FAILED` — Ansible killed the async task

---

## Step 6 — Cross-reference git: what code actually ran

```bash
git fetch --all --quiet

# What commits are in the branch but not in what ran?
git log --oneline <scm_revision>..<branch> -- <affected_file>

# Has a known fix been applied?
git branch -r --contains <fix-sha>
```

Always report:
- What commit SHA ran
- What's at HEAD of the branch now
- Whether any fix commits postdate what ran

---

## Step 7 — Synthesise findings

Produce a table of all reviewed runs:

| Job | Status | Duration | Cluster | Root cause / Notes |
|---|---|---|---|---|

Then answer:
1. **What's validated?** — which code paths have clean runs as evidence
2. **What failed and why?** — root cause per failure, one line each
3. **Is any failure still open?** — unfixed bugs vs already-fixed bugs
4. **What's untested?** — code paths that haven't been exercised yet
5. **Documentation & state updates** — what needs updating in project docs or PR evidence

---

## Step 8 — Update documentation & state

After any review that produces new evidence:

- Move newly-validated items from "pending" to "validated" in tracking docs
- Add new data points from job observations
- Close open questions that now have answers
- Document new failure modes or open questions surfaced by the run

**The discipline:** update project documentation immediately while the evidence is fresh, not later.

---

## Failure pattern reference (this repo)

If the root cause isn't clear from events or stdout signals, read:
[../aap-troubleshoot/references/failure-patterns.md](../aap-troubleshoot/references/failure-patterns.md)
(grouped by elapsed time — fast / boot / install-time / severe / infra)
