# What's Next

Tracking active tasks being carried out and their associated commits.

> **Purpose**: This file is the operational task list—what's actively being worked on right now. Each task should reference the commit hash when changes are made, providing a clear audit trail from task to code. For higher-level tracking, see `/BACKLOG.md`.

---

## Current Tasks

| Task | Status | Commit |
|------|--------|--------|
| — | — | *No active tasks* |

---

## Active Workstreams

*Create subdirectories under `.planning/` for each project workstream.*

| Workstream | Directory | Status |
|------------|-----------|--------|
| — | — | *No active workstreams* |

---

## Recently Completed

- Reorganized planning/tracking file locations
- Added purpose documentation to BACKLOG, ACTIVITY, whats-next
- Defined co-creation behaviors (Storm Session, Pre-Mortem, Smooth Brain, Progressive Bookkeeping)
- Created `.wip/` directory and `workstyle/working_style.md`
- Created `.cursor/rules/co-creation.mdc`

---

# Dead Drop — 2026-04-22

**In progress:** Storm Session on workspace behaviors (`.wip/workspace-behaviors/`) — session paused, not graduated

**Just completed:**
- Added Dead Drop behavior with Bread Crumb alias (944080e)
- Added Close-out compound behavior under Dead Drop (6a0873b)
- Added Conventions section: Isolation, Append-Don't-Replace (6a0873b)
- Updated Cursor rule to reflect all behaviors and conventions

**Next step:** Decide whether to graduate/discard the Storm Session, or continue adding behaviors. Roadmap Phase 3 mentions behavioral profiles (Architect, DevOps, etc.) as a future direction.

**Key decision:** Close-out is a compound behavior (Smooth Brain → Dead Drop), not a separate table entry. Conventions are always-on rules, distinct from invocable behaviors.

**Git state:** 6a0873b — Add Dead Drop behavior and conventions

**Open threads:** Storm Session `.wip/workspace-behaviors/` still exists with scratch files; can be resumed or discarded
