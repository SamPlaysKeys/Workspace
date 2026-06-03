# Conventions Section (draft for working_style.md)

## Conventions

Conventions are always-on rules that shape how work happens. Unlike behaviors, they're not invoked — they apply automatically.

### Isolation

Artifacts created during a session stay in the current project. No automatic write-back to external workspaces.

This workspace may reference or generate content for other repositories, but changes to external projects require explicit confirmation. When in doubt, ask before writing outside the current workspace.

**Exception:** Intentional cross-repo work (e.g., "generate a README for repo X") is permitted when explicitly requested.

### Append, Don't Replace

State files accumulate entries rather than being overwritten. The most recent entry is the active state, but history is preserved.

**Applies to:**
- `BACKLOG.md` — Dead Drops append, don't replace
- `discussion.md` in Storm Sessions — running log, not overwritten
- Any file serving as a session log or state tracker

**Why:** Provides audit trail and context. A fresh session can scan previous entries to understand trajectory, not just current state.

---

# Close-out Addition (draft for Dead Drop section)

#### Close-out (compound behavior)

**Trigger:** "close-out", "write a handoff", "I'm closing this session"

Close-out is a compound behavior for session endings. It combines Smooth Brain and Dead Drop with a specific focus on *continuation*.

**Flow:**
1. **Smooth Brain (scoped):** What's in this context that won't survive? What's assumed captured but isn't? What would a fresh session need to ask again?
2. **Dead Drop (for continuation):** Write a handoff oriented toward picking up, not just summarizing what happened.

Use Close-out when the session is ending but the work isn't finished. A fresh session should be able to continue without re-litigating settled decisions.
