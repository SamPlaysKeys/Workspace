---
name: aap-verification-gate
description: >
  Determine merge-readiness from AAP evidence by comparing what commit ran in AAP
  vs the current branch tip, then output a clear PASS/CONDITIONAL/BLOCK gate.
  Use when: user asks "is this verified?", "is this merge-ready?", "is PR up to
  date with successful runs?", or needs explicit rerun requirements. Keywords:
  verification gate, merge gate, scm_revision, branch tip, stale evidence,
  pre-merge validation, AAP evidence.
license: Apache-2.0
compatibility: >
  Requires network access to your AAP instance, plus git.
  Uses scripts/ helpers and ~/.config/aap/ credentials.
metadata:
  author: platform-team
  version: "1.0"
allowed-tools: Bash Read
---

# AAP Verification Gate

Use this skill to convert raw job outcomes into a merge gate decision.

## Prerequisites

Credentials at `~/.config/aap/token` and `~/.config/aap/hostname`.
If missing/empty, stop and ask user to run `./scripts/aap-setup`.

## Step 1 — Collect evidence jobs

Gather the job IDs relevant to the gate (typically latest phase_3 and phase_5).
If user provided only one ID, include it and explicitly call out missing required runs.

## Step 2 — Capture what code ran

For each job:

```bash
~/.config/aap/aap-api GET /api/v2/jobs/<id>/
```

Extract at minimum:
- `status`
- `elapsed`
- `name` / `playbook`
- `scm_branch`
- `scm_revision`
- cluster identifier from `extra_vars` where possible

Prefer `jq` for parsing. If Python is needed, use `python3` (never `python`).

## Step 3 — Compare against current remote branch tip

```bash
git fetch --all --tags --quiet
git rev-parse origin/<branch>
```

For each evidence job, determine:
- `job_sha == branch_tip_sha` (fresh evidence)
- `job_sha != branch_tip_sha` (stale evidence)

## Step 4 — Apply gate decision rules

Use this strict policy:

1. **PASS**
   - All required runs succeeded, and all were executed on the current branch tip.
2. **CONDITIONAL**
   - A successful run exists but is on an older SHA than branch tip.
   - Report exactly which reruns are required.
3. **BLOCK**
   - Required run failed on tip, or required run has not succeeded yet.

Never claim merge-ready when evidence SHA differs from branch tip.

## Step 5 — Output format (required)

Produce:

| Check | Evidence job | Job status | Job SHA | Branch tip SHA | Result |
|---|---|---|---|---|---|

Then:

- `Gate verdict: PASS | CONDITIONAL | BLOCK`
- `Required before merge: <explicit list of runs still needed>`
- `Evidence freshness: fresh | stale`

## Step 6 — Hand off to related skills

- For one failed job root cause: use **`aap-troubleshoot`**.
- For many runs/patterns: use **`aap-branch-review`**.
