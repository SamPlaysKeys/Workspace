---
name: decision-trail-update
description: >
  Post or refresh a PR "Decision trail update" comment using branch diff and AAP
  run evidence. Use when: documenting why changes were made, keeping a running
  evidence trail on a PR, summarising what is validated, and calling out what
  remains untested before merge. Keywords: decision trail, PR comment, AAP
  evidence, branch evidence, validated paths, untested paths, QA signal.
license: Apache-2.0
compatibility: >
  Requires gh CLI auth, git access to base/head refs, and AAP credentials
  (`~/.config/aap/token`, `~/.config/aap/hostname`) for run evidence.
metadata:
  author: platform-team
  version: "1.1"
allowed-tools: Bash Read
---

# Decision Trail Update

Use this skill to maintain a high-signal PR comment stream that explains
**decisions + evidence**, not just logs.

## Prerequisites

- GitHub auth (`gh auth status`)
- AAP credentials configured (`./scripts/aap-setup` if missing)

```bash
cat ~/.config/aap/hostname
wc -c < ~/.config/aap/token  # should be > 0
```

## Step 1 — Identify PR and baseline style

1. Read PR metadata and current body:
   ```bash
   gh pr view <pr_number> --json number,title,baseRefName,headRefName,body,url
   ```
2. Read existing PR comments and look for prior "Decision trail" style comments:
   ```bash
   gh api repos/<owner>/<repo>/issues/<pr_number>/comments --paginate
   ```
3. Reuse the established comment tone/structure when present.

## Step 2 — Capture full branch context (not session-only)

Summarize **all meaningful changes on the branch** (`base..head`) grouped by
themes (flow/order changes, validations, docs, etc.).

```bash
git fetch --all --quiet
git log --oneline <base_ref>..<head_ref>
git diff --name-only <base_ref>..<head_ref>
```

Do not post commit-by-commit enumeration unless explicitly requested.

## Step 3 — Pull run evidence for the branch

Query relevant job templates exercised by the changes on this branch:

```bash
scripts/aap-runs.py "<job_template_name>" <head_branch>
```

For selected jobs:
- Fast failures (<5 min): `scripts/aap-failures.py <id>`
- Long failures: `scripts/aap-scan-stdout.py <id> --context 3`

Extract one-line takeaways per run (cause + what it proves).

## Step 4 — Build the decision-trail comment

Use the template in `references/comment-template.md`.

Required sections:
1. **Decision trail update** heading with date
2. **Recent branch changes** (theme summary)
3. **Recent AAP evidence** table
4. **What this validates now**
5. **What remains untested**
6. **Verification / QA signal**:
   - `Status` (`pass|conditional|block|unknown`)
   - whether latest head SHA was exercised
   - required-before-merge action(s), if any

## Step 5 — Draft to file first (mandatory)

```bash
cat > /tmp/decision-trail-draft.md <<'EOF'
<comment markdown>
EOF
```

Never post directly from generated text without a draft file.

## Step 6 — Human review gate (mandatory)

1. Show the draft content to the user before posting.
2. Ask for explicit approval or edits.
3. If edits are requested, update the draft file and re-show.
4. Only continue after explicit approval.

## Step 7 — Post approved draft safely

```bash
gh pr comment <pr_number> --body-file /tmp/decision-trail-draft.md
gh api repos/<owner>/<repo>/issues/<pr_number>/comments --paginate | jq '.[-1]'
rm -f /tmp/decision-trail-draft.md
```

## Guardrails

- Evidence first; avoid generic status text.
- Link jobs (`https://aap.example.com/#/jobs/playbook/<id>/output`) instead of pasting
  large logs.
- Distinguish **code-path validation** from **environment/input blockers**.
- If no new evidence exists, post a short "no evidence delta" update.
- Keep detailed deep-dive analysis in targeted PR comments; keep the decision-trail
  comment as a crisp executive thread.
- Default flow is **draft file -> user review -> post**. Do not skip the review gate.
