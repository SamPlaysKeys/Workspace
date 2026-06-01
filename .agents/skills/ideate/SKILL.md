---
name: ideate
description: Quarantined workspace for brainstorming and rapid ideation. Auto-triggers when the user says "storm session", "storm sesh", "brainstorm", or "let's ideate".
---

# Ideate (Storm Session)

<objective>
Provide a safe, quarantined space for exploring net-new ideas without risking accidental changes to established code or documentation. All work must remain in the `wip/` directory until explicitly graduated.
</objective>

## Triggers
- User says "start a storm session" or "storm sesh"
- User says "let's brainstorm" or "ideate"
- User wants to explore a complex new feature before committing to an architecture.

## The Quarantine Rule
**CRITICAL**: While the `ideate` skill is active, you are strictly forbidden from modifying existing source code, production files, or files in `docs/` or `library/`. 
All scratchpad work, drafts, and code experiments MUST be written to `wip/<topic>/`.

## Process

1. **Identify Topic**: If the user hasn't provided a topic name, ask for a short hyphenated name (e.g., `api-gateway-design`).
2. **Scaffold Directory**: Create `wip/<topic>/context.md` and `wip/<topic>/discussion.md`.
3. **Acknowledge Quarantine**: Inform the user that the Storm Session is active and all work is safely quarantined to the `wip/` directory.
4. **Iterate**: Work with the user, capturing ongoing state in `context.md` and conversational/draft notes in `discussion.md`.
5. **Graduate**: When the user says the idea is ready, ask where they want the artifacts moved (e.g., into source code, or to the `document` skill for a formal write-up), then clean up the `wip/` folder.

## Templates

### 1. context.md
**Path**: `wip/<topic>/context.md`
**Usage**: Update this progressively as major decisions are made. It should allow a resumed session to know exactly where the idea stands.

```markdown
# Context: [Topic Name]

**Goal**: [What are we trying to figure out or create?]
**Current State**: [Where did we land? What's been decided?]
**Open Questions**: [What still needs to be resolved?]
**Key Constraints**: [Anything that limits options]
```

### 2. discussion.md
**Path**: `wip/<topic>/discussion.md`
**Usage**: A running, append-only log of ideas, proposals, and exchanges.

```markdown
# Running Discussion

## [YYYY-MM-DD HH:MM]
[Brief note on the latest idea, sketch, or proposal explored]
```