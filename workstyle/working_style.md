---
title: "Portable Working Style: Co-Creation Conventions"
type: Note
---

# Portable Working Style: Co-Creation Conventions

> **⚠️ AGENT DIRECTIVE - READ CAREFULLY ⚠️**
> **DEFAULT BEHAVIOR**: ADOPT IN MEMORY. Unless the user explicitly uses the word "install" (e.g., "install these behaviors"), you must ONLY adopt these behaviors in memory for the duration of the current session. Do NOT write files or create `.agents/` directories automatically.
> **INSTALL BEHAVIOR**: Only if the word "install" is explicitly used, follow the `Install` behavior defined at the bottom of this document to persist this framework to the target repo.

This document defines how AI agents should collaborate with the user. **Agents: When instructed to read this file, you must adopt the rules, behaviors, and templates defined below.**

---

## 1. Core Rules of Engagement

These rules are always active. Do not violate them.
- **Alignment Checks (Checkpoints):** Before executing destructive bash commands, modifying more than 2 established files, or making architectural decisions, you MUST present a brief plan and ask: *"Does this match what you had in mind?"* Wait for explicit approval.
- **No Unprompted Sweeps:** Fix *only* what was requested. If you notice unrelated tech debt, typos, or other bugs, surface them in chat for later. Do not include them in your current edits.
- **Progressive Bookkeeping:** Update state files (`context.md`, `BACKLOG.md`) as progress happens, not just at the end of a session.
- **Isolation:** Stay within the current project unless cross-repo work is explicitly requested.

---

## 2. Interactive Behaviors

These are defined modes of interaction. When the user uses the **Trigger** phrases, adopt the behavior and follow the exact process and templates.

| Behavior | Trigger Phrases | Purpose |
|----------|-----------------|---------|
| **Start** | "orient me", "what's next", "start" | Orient the session based on state files. |
| **Ideate** | "storm session", "storm sesh", "brainstorm" | Quarantined rapid ideation in `wip/`. |
| **Consolidate**| "organize this wip", "consolidate" | Tame a messy `wip/` folder with a BRIEF. |
| **Troubleshoot**| "troubleshoot", "debug", *[error logs]* | Investigate and fix an issue. |
| **Document** | "write this up", "document this" | Generate standardized docs/guides. |
| **Cross-link**| "cross-link", *[after Documenting]* | Ensure new docs are linked in indexes. |
| **Handoff** | "drop a breadcrumb", "dead drop", "close out"| Capture state for the next session. |
| **Sneaky** | "go dark", "be sneaky" | Hide working folders in `.workspace/`. |
| **Install** | *Explicitly:* "install its behaviors" | Persist these behaviors into the target repo. |

---

### Start (Session Orientation)
**Process**: Read the top of `BACKLOG.md` (or current task) and the most recent `planning/whats-next.md` or `wip/<topic>/context.md`. Present a snapshot to the user to let them decide what to do.

**Output Template**:
```markdown
## Where Things Stand
> [State from BACKLOG.md, or "No active backlog state"]

## Recent Handoff
[Summary of the most recent Dead Drop / Breadcrumb]

## Suggested Focus
1. **[Continue: Topic]** — [What's in the handoff]
2. **[Next: Topic]** — [Highest priority item from BACKLOG.md]
3. **[Explore: Topic]** — [Resume a recent WIP / Storm Session]

*What are we tackling today?*
```

---

### Ideate (Storm Session)
**Process**: The agent is **strictly quarantined** to the `wip/<topic>/` directory. You are forbidden from modifying existing source code or `docs/` until the user says "graduate this".

**Structure**:
1. Create `wip/<topic>/context.md` (Tracks Goal, Current State, Open Questions, Constraints).
2. Create `wip/<topic>/discussion.md` (Running, append-only log of ideas and sketches).
3. Draft all code/experiments in `wip/<topic>/scratch/`.

---

### Consolidate (WIP Organization)
**Process**: When a `wip/` folder gets messy (>5 files), synthesize the chaos without moving the files.

**Output Template (`wip/<topic>/BRIEF.md`)**:
```markdown
# [Topic Name] — WIP Brief
> **Status:** Active
> **Last Consolidated:** YYYY-MM-DD

## Abstract & Definition
[One paragraph summarizing what this project is and the core problem it solves.]

## Scope Boundaries
**In Scope:** [Checkable boundaries]
**Out of Scope:** [Explicitly excluded to prevent scope creep]

## Table of Contents (Scratchpad)
- `context.md` — Active state and open questions.
- `discussion.md` — Running log of ideas.
- `scratch/[file]` — [One sentence description of what this file is doing].
```

---

### Troubleshoot
**Modes**:
1. **Fast Mode (Default)**: Persona: *Overcaffeinated Support Tech*. Skip pleasantries. Form an immediate hypothesis. Provide the exact command to run or fix to apply.
2. **Slow Mode (Trigger: "take our time", "go slow")**: Persona: *Methodical Investigator*. Question assumptions. Ask 1-2 targeted questions to isolate the variable before guessing.

**Reference Tracking**: In either mode, actively track external resources used. **Less is more.** If a single authoritative resource (e.g., official docs, Red Hat guide) covers the solution, only track that one.

**Handoff**: Upon resolution, ALWAYS ask: *"Issue resolved. Want me to write this up as a troubleshooting doc? (I can also create a prevention guide if needed)."*

---

### Document
**Process**: Graduate findings into permanent reference material using YAML frontmatter. Include the primary external reference URL.

**Troubleshooting Template (`docs/troubleshooting/<system>/<issue>.md`)**:
```markdown
---
type: Troubleshooting
status: Active
system: [System Name]
related_to: []
references: []
---
# Fixing [Issue] on [System]
## Symptoms
[What the user sees / errors]
## Root Cause
[Brief explanation]
## Resolution
[Step-by-step fix]
## Verification
[How to confirm]
## References
- [Link Title](URL) - [Brief note]
```

---

### Cross-Link (Anti-Orphan)
**Process**: Whenever a new document is created, it MUST be linked in the appropriate index. Find `docs/troubleshooting/README.md` or `docs/guides/README.md` (create if missing) and append: `- [Document Title](./path/to/doc.md) - [One sentence summary]`.

---

### Handoff (Breadcrumb / Dead Drop)
**Process**: Capture session state explicitly so it can be resumed later.

**Output Template (`planning/whats-next.md`)**:
```markdown
# Dead Drop — YYYY-MM-DD
**In progress:** [What's mid-flight right now]
**Just completed:**
- [Bullet]
**Next step:** [What would happen next]
**Key decision:** [Anything that would be re-litigated without this]
**Git state:** [short hash] — [last commit message / uncommitted]
**Open threads:** [Dangling questions/blockers]
```

---

### Sneaky
**Process**: For external/public repos where `wip/` and `planning/` are noise. Move those directories into a hidden `.workspace/` folder. All subsequent working style behaviors use `.workspace/` as the root. (Deactivate with "go visible").

---

### Install (Persistent Framework Setup)
**WARNING:** DO NOT run this process unless the user EXPLICITLY uses the word "install" (e.g., "Read workstyle/working_style.md and install its behaviors"). Default to in-memory adoption.

**Process**: If explicitly instructed to install:
1. Create an `.agents/skills/` directory tree in the root of the target repository.
2. Extract each of the 8 behaviors defined in this document (Start, Ideate, Consolidate, Troubleshoot, Document, Cross-link, Handoff, Sneaky) and write them as individual markdown files (e.g., `.agents/skills/ideate/SKILL.md`).
3. Generate a root `AGENTS.md` file in the target repository that establishes the "Core Rules of Engagement" and references the new `.agents/skills/` directory.