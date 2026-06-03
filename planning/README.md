# Planning

Session state and operational tracking for the workspace.

## Files

| File | Purpose |
|------|---------|
| `ACTIVITY.md` | Log of structural and meta changes to the workspace |
| `roadmap/` | Strategic planning and long-term roadmaps |

## How It Works

### `BACKLOG.md` (in root)

This is where AI agents write **Dead Drops** — structured handoffs that capture:
- What's in progress
- What was just completed
- What the next step would be
- Key decisions that shouldn't be re-litigated

New entries append to the file (don't replace) under the `## Active Sessions` header. A fresh session can scan recent entries to understand trajectory.

### `ACTIVITY.md`

Tracks structural changes to the workspace itself — new directories, reorganizations, convention changes. Less frequently updated than `BACKLOG.md`.

## For AI Agents

When resuming work:
1. Read `BACKLOG.md` to understand current state
2. Do a quick Smooth Brain check — what assumptions are being carried forward?
3. Ask the human what's changed since the last session

When ending work:
1. Write a Dead Drop to `BACKLOG.md` under `## Active Sessions`
2. Update any relevant `wip/` session files
