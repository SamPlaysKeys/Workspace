# Dead Drop (draft)

**Invocation**: "Dead Drop" or "Bread Crumb" (alias)

**Trigger**: User-initiated, or agent-initiated when session is ending.

Dead Drop is an explicit, structured capture of session state for handoffs. It leaves enough context that a future session (same human, different agent, or both) can pick up without re-litigating settled decisions.

## When to Use

- Mid-session when you need to pause but will return
- End of session as a handoff to future work
- Before context gets too long and risks being lost
- When switching between agents or tools

## Where It Lives

Dead Drops go in `.planning/whats-next.md`.

For Storm Sessions, also update the session's `context.md` — but the Dead Drop in `whats-next.md` is the canonical "resume here" marker.

## Format

### Full Dead Drop (preferred)

```markdown
# Dead Drop — YYYY-MM-DD

**In progress:** [one sentence — what's mid-flight right now]

**Just completed:**
- [bullet]
- [bullet]

**Next step:** [one sentence — what would happen next if the session continued]

**Key decision:** [one sentence — anything that would be re-litigated without this — or "none"]

**Git state:** [short hash] — [last commit message] (or "uncommitted changes" / "clean")

**Open threads:** [any dangling questions or blocked items — or "none"]
```

### Quick Bread Crumb (fallback)

When there's no time for the full format:

```markdown
> YYYY-MM-DD HH:MM — [what's happening / what's next]
```

Quick crumbs append to the file. A future session can read the trail and reconstruct. **This is a fallback, not default** — use full format when possible.

## Agent-Initiated

The agent should prompt for a Dead Drop when:
- Session is clearly ending (user says goodbye, wrapping up)
- Context is getting long and state should be preserved
- Switching to a different workstream

Phrase it as: "Want me to drop a bread crumb before we wrap?"

## Recovery

If a session ends without a Dead Drop, the git log is the fallback — shows what landed, not what was in flight. Clean working tree + recent commits = recoverable state, just less context.
