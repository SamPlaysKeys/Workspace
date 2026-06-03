# Discussion

## 2026-04-22

### Session Start

Starting Storm Session on workspace behaviors. User has ideas to drop in.

---

### Checkpoint Behavior

**Source:** User dropped reference from hhellbusch/my-ai-workspace in `scratch/checkpoint.md`

**Discussion:**
- Pattern is about structured session state capture for handoffs
- Complements Progressive Bookkeeping (always-on) with explicit snapshots
- Decided to add as its own behavior (option 1), not fold into Progressive Bookkeeping

**Naming:**
- Primary: **Dead Drop** — spy term, leaving intel for the next agent
- Alias: **Bread Crumb** — "drop a bread crumb" also triggers this
- Can swap primary/alias later based on natural usage

**Decision:** Add Dead Drop as fifth behavior with Bread Crumb alias.

---

### Session State Conventions

**Source:** User dropped `scratch/session_state.md`

**Concepts identified:**

1. **Isolation** — Artifacts stay in this project; no write-back to external workspaces
2. **Close-out mode** — Specific end-of-session flow: Smooth Brain → Dead Drop (for continuation)
3. **Append, don't replace** — State files accumulate; most recent entry is active

**Discussion:**

These aren't all behaviors — some are conventions (rules that apply always, not invoked).

| Concept | Type | Notes |
|---------|------|-------|
| Isolation | Convention | Applies to cross-repo work; prevents leakage |
| Close-out | Compound behavior | Smooth Brain + Dead Drop, triggered at session end |
| Append-don't-replace | Convention | For state files like BACKLOG.md |

**Open questions:**
- Should Close-out be a named behavior, or just documented as a pattern?
- Where do conventions live in working_style.md? New section?
- Does isolation need exceptions for intentional cross-repo work?

---

## Conventions (working through)

Conventions are always-on rules, not invoked. They shape how work happens regardless of which behavior is active.

### Candidate conventions:

**1. Isolation**
> Artifacts created during a session stay in the current project. No automatic write-back to external workspaces.

*Why it matters:* This workspace references/manages other repos. Without isolation, an agent might accidentally modify an external project when working here.

*Exception:* Intentional cross-repo work (e.g., generating docs for another repo) requires explicit confirmation.

**2. Append, don't replace**
> State files (BACKLOG.md, session logs) accumulate entries. Most recent is active. Don't overwrite history.

*Why it matters:* Provides audit trail. A session can scan previous entries to understand trajectory, not just current state.

*Applies to:* `BACKLOG.md`, Dead Drops, any running log

**3. Close-out** (resolved → compound behavior)
> When ending a session, run: Smooth Brain (what won't survive?) → Dead Drop (for continuation, not summary).

*Why it matters:* Regular Dead Drop captures state. Close-out specifically asks "what would a fresh session need to pick up without re-asking?"

*Trigger phrases:* "close-out", "write a handoff", "I'm closing this session"

**Decision:** Compound behavior, documented under Dead Drop (not its own table row). Close-out = Smooth Brain + Dead Drop triggered together.

---

### Decisions Summary

1. **Close-out**: Compound behavior under Dead Drop, not separate table entry
2. **Conventions section**: Add to working_style.md, separate from Behaviors
3. **Conventions to add**: Isolation, Append-don't-replace

---

## 2026-04-23

### Troubleshoot Behavior

**Context:** User wants to use workspace for troubleshooting external systems (OpenShift, operators, containers). Needs to capture remediation knowledge in `docs/`.

**Initial question:** Behavior or convention?

**Discussion:**
- Convention would encroach on existing behaviors:
  - Storm Session trigger is explicit; troubleshooting often implicit ("I'm getting this error...")
  - Storm Session framing is "disposable brainstorming"; troubleshooting builds reference material
  - Progressive Bookkeeping doesn't capture troubleshooting-specific state (hypotheses, what we've tried, ruled out)
  - Dead Drop handoffs look different for troubleshooting
- If documenting as convention requires "it's like Storm Session but..." — that's a hidden behavior

**Decision:** Behavior, not convention.

**Key design points:**
- Lightweight session structure: single `investigation.md` file, updated progressively
- Structured running log: Symptoms → Investigation (hypothesis/tried/result) → Resolution
- **Graduation step:** Optional prompt to create distributable docs:
  - **Remediation walkthrough** — "how to fix this issue" (for others hitting same problem)
  - **Setup/knowledge doc** — "how to set it up right" (avoid the issue entirely)
- Distributables go to `docs/troubleshooting/<system>/` or `docs/guides/<system>/`

**Draft:** `scratch/troubleshoot-behavior.md`

**Open questions (resolved):**

1. **investigation.md graduation?** → Disposable. Delete with session once docs exist.

2. **Tags/categories for findability?** → Deferred. Added to BACKLOG Ideas/Future for later.

3. **Quick-fix path?** → Yes. Two modes:
   - Full session for actual troubleshooting (investigation needed)
   - Quick doc path for known fixes or "write up how to do X" requests — skip session, write directly to docs/

**Additional refinement:**
- Graduation always creates remediation doc (Symptoms, Investigation, Resolution)
- Prevention doc: agent creates if clearly needed, prompts user if unsure, never skips without user saying so

**Graduated:** Added to `workstyle/working_style.md` and `.cursor/rules/co-creation.mdc`

---

### Open Item: Auto-load Working Style for CLI Agents

**Context:** User wants to explore how to auto-load `workstyle/working_style.md` when launching CLI agents (Cursor, Gemini, OpenCode, Copilot) in this repo.

**To explore:**
- What mechanisms do each CLI agent support for auto-loading context?
- AGENTS.md / CLAUDE.md / similar conventions?
- Repo-level config files per tool?
- Wrapper scripts?

**Status:** Parked for future session.

---
