# Planning

Session state and operational tracking for the workspace.

## Files

| File | Purpose |
|------|---------|
| `whats-next.md` | Dead drops, current work, session handoffs — **the resume point** |
| `ACTIVITY.md` | Log of structural and meta changes to the workspace |

## How It Works

### `whats-next.md`

This is where AI agents write **Dead Drops** — structured handoffs that capture:
- What's in progress
- What was just completed
- What the next step would be
- Key decisions that shouldn't be re-litigated

New entries append to the file (don't replace). A fresh session can scan recent entries to understand trajectory.

### `ACTIVITY.md`

Tracks structural changes to the workspace itself — new directories, reorganizations, convention changes. Less frequently updated than `whats-next.md`.

## For AI Agents

When resuming work:
1. Read `whats-next.md` to understand current state
2. Do a quick Smooth Brain check — what assumptions are being carried forward?
3. Ask the human what's changed since the last session

When ending work:
1. Write a Dead Drop to `whats-next.md`
2. Update any relevant `wip/` session files
