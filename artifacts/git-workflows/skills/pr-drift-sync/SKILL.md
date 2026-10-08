---
name: pr-drift-sync
description: >
  Keep pull request title/description aligned with the real branch state and
  latest verification evidence. Use when: user asks if PR title/body is still
  accurate, asks to update PR summary after many commits, or wants PR metadata
  refreshed from current diffs and run evidence. Keywords: PR drift, PR title,
  PR description, synchronize PR, stale PR body, gh pr edit.
license: Apache-2.0
compatibility: >
  Requires gh CLI auth to the repo and git access to the branch/base refs.
metadata:
  author: platform-team
  version: "1.0"
allowed-tools: Bash Read
---

# PR Drift Sync

Refresh PR title/body so it reflects current code and verification reality.

## Step 1 — Gather PR + branch truth

```bash
gh pr view <id> --json title,body,headRefName,baseRefName,headRefOid
git fetch origin --quiet
git log --oneline origin/<base>..origin/<head>
git diff --name-only origin/<base>..origin/<head>
```

## Step 2 — Build updated metadata

Use the actual diff + commit themes to produce:
- accurate title scope
- summary of meaningful technical changes
- current verification status and explicit remaining gates

Avoid stale role names, stale SHA references, and outdated "status" blocks.

## Step 3 — Safe update command pattern (required)

Always update body via stdin/file, never inline shell interpolation for markdown:

```bash
cat <<'EOF' | gh pr edit <id> --title "<new-title>" --body-file -
<new body markdown>
EOF
```

This avoids quoting/backtick expansion bugs and interactive hangs.

## Step 4 — Verify applied state

```bash
gh pr view <id> --json title,body,url
```

Confirm title/body exactly match intended content.

## Step 5 — Tooling policy

- Prefer `jq` for JSON extraction where needed.
- If Python fallback is required, use `python3` explicitly.
- If required tools are missing (`gh`, `jq`, `python3`), stop and ask user to
  install/provide them before proceeding.
