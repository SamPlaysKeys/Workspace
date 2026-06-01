---
name: start
description: Quick session orientation — surfaces state, recent handoffs, and suggests focus. Auto-triggers on "where were we", "what's next", or explicitly starting a session.
---

# Start — Session Orientation

<objective>
Provide a structured snapshot of the workspace to help the user decide what to work on. This prevents the agent from guessing the next task and puts the user in control.
</objective>

## Triggers
- User says "start", "what's next", "where were we", or "orient me".
- First interaction in a new working session.

## Process

1. **Read Backlog State**: Read the top of `BACKLOG.md` (specifically the `> State:` line or current active task).
2. **Read Handoff**: Look for the most recent `planning/whats-next.md` or `wip/<topic>/context.md` that was updated.
3. **Format Output**: Use the strict template below to present a compact snapshot. Do NOT invent tasks. 

## Template

```markdown
## Where Things Stand
> [State line from BACKLOG.md, or "No active backlog state"]

## Recent Handoff
[One paragraph summary of the most recent Dead Drop / Breadcrumb]

## Suggested Focus
1. **[Continue: Topic]** — [What's in the handoff]
2. **[Next: Topic]** — [Highest priority item from BACKLOG.md]
3. **[Explore: Topic]** — [Resume a recent WIP / Storm Session]

*What are we tackling today?*
```