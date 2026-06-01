---
name: consolidate
description: Organizes a messy WIP/Storm Session folder into a structured Abstract and Table of Contents. Auto-triggers on "consolidate this", "organize the wip", or "create a brief".
---

# Consolidate — WIP Organization

<objective>
Keep work safely quarantined in `wip/` while preventing it from becoming an unmanageable mess. Generates a `BRIEF.md` within the `wip/<topic>/` folder to act as an abstract, scope definition, and Table of Contents for the scratchpad.
</objective>

## Triggers
- User says "consolidate this wip", "organize this", or "make a brief".
- A `wip/` directory accumulates more than 5 files or becomes difficult to navigate.

## Process

1. **Scan the WIP**: Analyze `wip/<topic>/discussion.md`, `context.md`, and all files in `scratch/`.
2. **Synthesize**: Extract the core problem statement, the decided scope, and catalog the existing files.
3. **Generate Brief**: Create or update `wip/<topic>/BRIEF.md` using the template below.
4. **Clean up**: Offer to archive or delete dead-end scratch files that are no longer relevant to the consolidated brief.

## Template: `wip/<topic>/BRIEF.md`

```markdown
# [Topic Name] — WIP Brief

> **Status:** [Active / Paused / Ready to Graduate]
> **Last Consolidated:** [YYYY-MM-DD]

## Abstract & Definition
[One paragraph summarizing what this WIP project actually is, why it exists, and the core problem it solves.]

## Scope Boundaries
**In Scope:**
- [Checkable boundary]
- [Checkable boundary]

**Out of Scope:**
- [Explicitly excluded to prevent scope creep]

## Table of Contents (Scratchpad)
*An index of the active working files in this session.*

- `context.md` — Active state and open questions.
- `discussion.md` — Running log of ideas.
- `scratch/[filename.ext]` — [One sentence description of what this file is doing/testing].
- `scratch/[filename.ext]` — [One sentence description of what this file is doing/testing].
```