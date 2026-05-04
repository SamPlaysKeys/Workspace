## Session state conventions

**Isolation:** All artifacts created during this session stay in this project. This document contains no references to any external workspace — artifacts here don't write back anywhere.

**Where things go:**
- Checkpoints and handoffs → `.planning/whats-next.md` (create `.planning/` if needed)
- If no `BACKLOG.md` exists: create one with `## In Progress`, `## Up Next`, `## Ideas` sections
- Commits go to the local repository

**Close-out mode** (window closing, work may or may not be done): Close-out means this context window is ending — not that the work is finished. A fresh session may need to continue from here.

Skip spar — there's no approach to challenge. Run shoshin before writing the handoff:

> "What's in this context window that won't survive the reboot? What have I assumed is captured that isn't? What would a fresh session need to ask for again to pick this up without relitigating?"

Check the stack for open threads — those belong in the handoff. Then write a checkpoint or quick capture oriented toward continuation, not just summary.

Trigger: "close-out" / "write a handoff" / "I'm closing this session."

When the session state file already has content from a previous context, append with a datestamp — don't replace. The most recent entry is the active state.

**On drift:** This document is a snapshot of a working style that evolves. If something feels off or outdated, re-copy from the source workspace. The version date above indicates how current this snapshot is.
