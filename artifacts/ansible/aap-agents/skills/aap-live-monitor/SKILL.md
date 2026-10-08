---
name: aap-live-monitor
description: >
  Live validation of a running AAP job: hard failures plus semantic correctness of
  changed code sections. Use when: a job is currently running on aap.example.com or
  aap-test.example.com; proactive validation while job is in progress; checking for
  silent failures (vacuous asserts, loop filter misses, wrong output values, missing
  hosts). Use aap-troubleshoot for post-failure root cause analysis instead.
  Keywords: live monitor, running job, in-progress, aap running, job status, check
  status, how's it going, silent failure, semantic validation.
license: Apache-2.0
compatibility: >
  Requires network access to your AAP instance, plus git.
  Helper scripts live in scripts/. Run scripts/aap-setup to initialise credentials.
metadata:
  author: platform-team
  version: "1.0"
allowed-tools: Bash Read
---

# AAP Live Monitor

Live validation of a **running** AAP job: hard failures + semantic correctness of
changed code sections. Catches bugs that Ansible reports as `ok` — silent loop
filter misses, wrong output values, vacuous asserts, missing hosts in a loop.

Use `aap-troubleshoot` for post-failure root cause; this skill is for proactive
validation while the job is still running.

**Agent model:** this agent is **on-demand, not background-watching**. It acts
when the user sends a message — it does not autonomously poll between turns.
`aap-poll.sh` runs as a detached OS process giving the user a live heartbeat;
the agent validates on request. Typical triggers:

- *"check status"* / *"how's it going?"* → run Step 3
- Job completes or fails (shell notification or user message) → run Step 4 / switch to `aap-troubleshoot`

---

## Step 0 — Verify target before doing anything else

```bash
~/.config/aap/aap-api GET /api/v2/jobs/<id>/ | jq '{status, playbook, scm_branch, scm_revision, elapsed}'
```

- If `status != "running"` → skip to **Step 4** (or use `aap-troubleshoot` if failed).
- If the job is part of a workflow, this skill expects a **leaf job ID**, not a
  workflow job ID. Workflow job IDs should be resolved via `scripts/aap-workflow.py`.
- Record `scm_revision` — use it as the diff base if the user hasn't specified one.

---

## Step 1 — Derive checkpoints from changed code

```bash
# Use scm_revision from Step 0 as the base; fall back to origin/<base-branch>
git diff <scm_revision>...HEAD -- '*.yml' '*.j2'   # hunks — shows what changed
git diff <scm_revision>...HEAD --name-only -- '*.yml' '*.j2'  # file list for scope
```

For each changed task/template, build one checkpoint row per logical change:

| # | Changed section | Task fragment (unique substring) | ✅ success evidence | ❌ silent-fail evidence | Expected count |
|---|----------------|----------------------------------|--------------------|-----------------------|----------------|
| 1 | `ResetConfig filter` | `Report controllers` | `eligible=True` | `eligible=False` or 0 results | 1 row per node |

Rules:
- **Task fragment must be unique enough** to not appear in unrelated tasks.
- **Expected count** makes host-count assertions explicit (e.g. "8 nodes").
- If the success/fail pattern is not observable in stdout, mark the checkpoint
  `Unobservable` — do not treat absence of failure as a pass.
- Add checkpoints for anything the user explicitly flags.

Store the checkpoint table in the session SQL db:
```sql
CREATE TABLE IF NOT EXISTS checkpoints (
  id INTEGER PRIMARY KEY, section TEXT, fragment TEXT,
  pass_pattern TEXT, fail_pattern TEXT, expected_count TEXT,
  status TEXT DEFAULT 'pending', notes TEXT
);
```

---

## Step 2 — Start poll loop (detached)

Run detached so it survives Escape or session resets:

```bash
scripts/aap-poll.sh <id> 30 > /tmp/poll-<id>.log 2>&1 &
echo "poll PID: $!"    # record PID; kill $(pgrep -f "aap-poll.sh <id>") to stop
```

`aap-poll.sh` adapts interval from event rate: `> 5 ev/s → min`, `1–5 → 30s`,
`0.1–1 → 60s`, `< 0.1 → 120s`. Use the `ev/s` column to judge how often to
run Step 3: stagnant phases don't need 2-minute refreshes.

### Alternative: `aap-stream.py` for interactive/foreground monitoring

`scripts/aap-stream.py <job-id>` is a developer/operator tool for watching a
job's raw stdout scroll in real time (WebSocket first, REST-polling fallback),
rather than the heartbeat-style summaries `aap-poll.sh` produces. Prefer it
when a human wants to watch full task output live in a terminal (e.g. `tmux`
pane) instead of periodic snapshots; it is not intended to replace the
detached poll-loop pattern above, since it runs in the foreground and blocks
the invoking shell. Use `--tail` to attach mid-job without replaying from the
start.

---

## Step 3 — Snapshot + validate (repeat while running)

```bash
python3 scripts/aap-scan-stdout.py <id> --no-cache 2>&1 | grep "lines$"
STDOUT=~/.cache/aap/<id>-stdout.txt
python3 scripts/aap-failures.py <id> --no-cache 2>&1   # hard failures first
grep -n "^TASK \[" "$STDOUT" | tail -10                 # current position
```

**If `aap-failures.py` shows a hard failure** → stop the semantic loop, switch to
`aap-troubleshoot`, and mark all unreached checkpoints `blocked`.

For each checkpoint whose task fragment has appeared in stdout, evaluate inside
its task block only (not the full file):

```bash
# Extract the specific task block, then grep inside it
sed -n '/TASK \[<fragment>/,/^TASK \[/p' "$STDOUT" | grep -E "<pass_pattern>|<fail_pattern>"
# For msg: fields specifically:
sed -n '/TASK \[<fragment>/,/^TASK \[/p' "$STDOUT" | grep '"msg"'
```

Count matches when `expected_count` is set. Assign status:
- ✅ **Pass** — pass pattern found, count met, no fail pattern
- ❌ **Fail** — fail pattern found, or count below expected
- ⚠️ **Warn** — ambiguous (e.g. pass pattern present but fewer hosts than expected)
- ⏳ **Pending** — task not yet reached in stdout
- 🔇 **Unobservable** — no reliable stdout signal; can't confirm either way

Report after each refresh:
```
@ <elapsed>s  ✅ [1] eligible=True×8  ⏳ [2] not reached  ⏳ [3] not reached
```

---

## Step 4 — Final report on completion

```bash
~/.config/aap/aap-api GET /api/v2/jobs/<id>/ | jq '{status, elapsed, host_status_counts}'
python3 scripts/aap-failures.py <id> --no-cache 2>&1
```

Scan complete stdout for any still-pending checkpoints, then produce:

```
Job <id> — <status> in <elapsed>s
  ✅ [1] <what passed and how — include counts>
  ❌ [2] <what was wrong>
  🔇 [3] <unobservable — explain why>
Overall: PASS / FAIL / PARTIAL
```

---

## Silent failure patterns

These are **checkpoint-specific** signals, not universal rules. Apply them when
deriving checkpoints in Step 1, not as generic post-hoc greps.

| Pattern in stdout | What it means |
|-------------------|--------------|
| `"skip_reason": "No items in the list"` | Loop filter returned zero items — task did nothing |
| Custom `eligible=False` debug output | Filter logic excluded items it should have kept |
| `changed=false` on a task that must mutate state (first run) | Action silently skipped |
| Fewer host results than inventory size | Some hosts dropped from the loop |
| `"All assertions passed"` on an assert with `default([])` guard | May be vacuously true — verify the input list was non-empty |
