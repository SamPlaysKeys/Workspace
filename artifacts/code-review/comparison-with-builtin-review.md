---
type: Note
date: 2026-11-11
---

# Smooth Brain Review vs. the built-in `/review` command

A quick comparison for anyone deciding which to use. OpenCode, and some other harnesses, ship a `/review` command by default. This skill does a different job.

**In short:** `/review` is a fast check of changes you just made. Smooth Brain Review is a slower, costlier audit of code whose behavior you don't want to take on trust.

> **Scope of this note:** the `/review` description is based on OpenCode's original template ([commit `6355ed6`](https://github.com/anomalyco/opencode/commit/6355ed6ae754d571b461c37cb576f29522a8accb), Dec 2025). It may have changed since, and other harnesses implement `/review` differently. Check your harness's current command before relying on this table.

## How each works

**`/review`** is a single prompt template, and it can be overridden with a custom `review.md`.
- One agent, running in a sub-session so the main context stays clean.
- Reviews changes only, not pre-existing code: uncommitted changes by default, or a commit, a branch compared with `HEAD`, or a PR.
- Focuses on bugs, then structure, then obvious performance problems.
- Conservative by design: "be certain before you flag", no style nits, no flattery.
- Runs inside the project, so it has the repo's instructions and may have the conversation's assumptions.

**Smooth Brain Review** is a procedure that orchestrates many agents ([SKILL.md](skills/smooth-brain-review/SKILL.md)).
- Six context-free delegates, each with a different lens, plus a blind read that describes the code with no intent given.
- Fresh verifiers try to refute the findings, and may run code in a temp directory.
- The main agent re-rates severity and reconciles the blind read against stated intent.
- Read-only. A cost gate and a git-repo check run before anything launches.

## Side by side

| | `/review` | Smooth Brain Review |
|---|---|---|
| **Question answered** | "Is this change correct?" | "What does this code actually do, and where does that differ from what we believe?" |
| **Target** | Diffs: uncommitted changes, a commit, a branch or a PR | Any code or directory, or a diff, branch or PR |
| **Reviewers** | One agent | Six context-free agents, one per lens (three in `quick` mode) |
| **Context** | The project's and, depending on the harness, the conversation's | None by design. The brief is neutral and intent is withheld |
| **Blind read** | No | Yes (the Smooth Brain delegate) |
| **Intent vs reality** | Not a step | Explicit reconcile against docs, PR text and prior beliefs |
| **Verification** | The agent judges its own confidence | Separate verifiers try to refute each finding and may run code |
| **Severity** | One agent's judgment | Re-rated by the main agent after adjudication |
| **Cost and speed** | One run: seconds to a couple of minutes | Up to about 14 subagent runs, behind a confirmation gate |
| **Invocation** | `/review [target]`, cheap to run often | Manual only (`@smooth-brain-review`), deliberately heavyweight ([ADR 0001](adr/0001-name-and-manual-only-invocation.md)) |
| **Output** | Chat | Chat, plus an optional file from a template |
| **Tuning** | Override the template, or set `model` and `agent` on the command | Choose the subagent and model per run |

## When to use which

| Situation | Use |
|---|---|
| Before a commit or PR, to catch bugs in what just changed | `/review` |
| Frequent, low-cost feedback while working | `/review` |
| Inheriting or auditing code, or checking whether docs and behavior drifted | Smooth Brain Review |
| A second opinion that the current conversation can't anchor | Smooth Brain Review |
| Findings that need evidence, such as a reproduced runtime failure | Smooth Brain Review |

## Failure modes

- `/review` can miss a bug that the whole conversation believes is fine, because the reviewer shares the same assumptions.
- Smooth Brain Review's delegates can't share those assumptions, but they can over-report. Verification and severity re-rating exist to correct that, at a much higher cost.

## Overlap

Both read the diff, look for bugs and security issues, and say when they are unsure. If only the bug-finding is needed, `/review` is enough. A middle ground is a custom command that runs the skill's `quick` mode.

See the [ADRs](adr/README.md) for why the skill is built the way it is.
