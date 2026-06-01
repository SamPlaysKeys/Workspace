---
name: handoff
description: Captures session state for future continuation. Auto-triggers when the user says "drop a breadcrumb", "dead drop", "close out", or asks to pause/wrap up.
---

# Handoff (Dead Drop / Breadcrumb)

<objective>
Create an explicit, structured capture of session state. Leave enough context so a future session (same human, different agent, or both) can pick up exactly where this one left off without re-litigating settled decisions.
</objective>

## Triggers
- User says "drop a breadcrumb" or "leave a breadcrumb".
- User says "make a dead drop" or "dead drop".
- User says "close out", "write a handoff", or "let's pause here".
- Agent detects the session is ending and context is long (proactively ask: "Want me to drop a breadcrumb before we wrap?").

## Process

1. **Assess State**: Briefly review what was just completed, what is currently in-flight, and any open threads or blockers.
2. **Check Git**: Note the current git state (uncommitted changes, or the last commit hash).
3. **Format**: Use the **Full Dead Drop** template by default. If the user explicitly asks for a "quick crumb", use the fallback.
4. **Save**: Append the output to `planning/whats-next.md`. (If in a Storm Session, also update the session's `context.md`).

## Templates

### 1. Full Dead Drop (Default)
**Path**: Append to `planning/whats-next.md`

```markdown
# Dead Drop — YYYY-MM-DD

**In progress:** [One sentence — what's mid-flight right now]

**Just completed:**
- [Brief bullet]
- [Brief bullet]

**Next step:** [One sentence — what would happen next if the session continued]

**Key decision:** [One sentence — anything that would be re-litigated without this — or "none"]

**Git state:** [short hash] — [last commit message] (or "uncommitted changes" / "clean")

**Open threads:** [Any dangling questions or blocked items — or "none"]
```

### 2. Quick Bread Crumb (Fallback)
**Use when**: User explicitly asks for a "quick crumb" or is in a rush.
**Path**: Append to `planning/whats-next.md`

```markdown
> YYYY-MM-DD HH:MM — [Brief note on what's happening and what's next]
```