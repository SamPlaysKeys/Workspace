---

### Checkpoints and session handoffs

**Checkpoint** (mid-session save, fast): Write to `BACKLOG.md` under `## Active Sessions`.

```
# Checkpoint — YYYY-MM-DD

**In progress:** [one sentence — what's mid-flight right now]
**Just completed:** [1-3 bullets]
**Next step:** [one sentence — what would happen next if the session continued]
**Key decision:** [one sentence capturing anything that would be re-litigated without knowing it was settled — or "none"]
**Git state:** [short hash] — [last commit message]
**Open threads:** [see stack tracking below — or "none"]
```

**Example:**

```
# Checkpoint — 2026-04-20

**In progress:** Refactoring the auth middleware to support token refresh
**Just completed:**
- Moved session store to Redis (a3f2c1d)
- Updated login handler to write refresh token (b9e4d2a)
**Next step:** Wire refresh endpoint, then update the client to retry on 401
**Key decision:** Refresh tokens stored in httpOnly cookies, not localStorage — XSS tradeoff settled
**Git state:** b9e4d2a — auth: update login handler for refresh token support
**Open threads:** none
```

**Quick capture** (time-short, no template): When there's no time for the full format, append two or three lines — no structure required:

```
> YYYY-MM-DD HH:MM — [what's happening / what's next]
```

Quick captures append to the file rather than replacing it. A future session can read the trail and reconstruct state well enough. **Quick capture is a fallback, not a default** — if the full format is possible, use it.

**Session handoff** (end-of-session, fuller): Same file, same location, more detail — add remaining gaps, framing decisions made, and context a fresh session would need to pick up without asking questions already answered.

**Recovery:** If a session ends without a checkpoint, the git log is the fallback. It shows what landed, not what was in flight — but clean working tree + recent commits = recoverable state.

---
