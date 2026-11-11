---
type: README-Note
---

# Architecture decision records: Smooth Brain Review

These ADRs record why the `smooth-brain-review` skill works the way it does. Read them before changing the skill, or when you are adapting it to a different environment.

| # | Decision | Status |
|---|---|---|
| [0001](0001-name-and-manual-only-invocation.md) | Name the skill "Smooth Brain Review" and make it manual-only | Accepted |
| [0002](0002-context-free-congress-of-lenses.md) | Review with a context-free congress of six lenses | Accepted |
| [0003](0003-withhold-intent-then-reconcile.md) | Withhold intent from reviewers, then reconcile against it | Accepted |
| [0004](0004-adjudication-wave-and-severity.md) | Verify findings with an adjudication wave, then re-rate severity | Accepted |
| [0005](0005-read-only-and-output-path-as-approval.md) | Stay read-only; report to chat; a stated path approves the write | Accepted |
| [0006](0006-cost-gate-and-git-repo-check.md) | Confirm before launch, and check the target is a git repo | Accepted |
| [0007](0007-skill-as-orchestrator-no-custom-agent.md) | Implement as a skill that orchestrates, not a custom agent | Accepted |

## Reading order
0001 and 0007 explain how the skill is invoked and packaged. 0002 to 0005 describe the review itself. 0006 is the safety gate in front of it.
