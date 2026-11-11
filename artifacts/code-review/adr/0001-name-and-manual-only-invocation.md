---
type: ADR
Status: Accepted
Date: 2026-11-11
---

# ADR 0001: Name the skill "Smooth Brain Review" and make it manual-only

## Context

"Smooth Brain" started as an informal working-style behavior: stop assuming, say what the code actually does in plain terms, and surface every assumption. It was never a standalone skill. The goal was a repeatable way to say "forget everything we believe about this code and review it with a critical eye."

Two constraints shaped how it is invoked:

- A full run is expensive. It launches 6 delegates plus up to 8 verifiers, so it must never fire on its own or be suggested unprompted.
- The phrase "smooth brain" already acts as a slow-mode trigger in a separate `troubleshoot` skill. A loose phrase would activate both.

## Decision

1. **Name:** `smooth-brain-review`, displayed as "Smooth Brain Review".
2. **Trigger word:** every natural-language trigger must contain the word **review**. A bare "smooth brain" does not trigger this skill.
3. **Manual only:** the skill sets `disable-model-invocation: true` in its frontmatter. It is hidden from the model's skill list and loads only when explicitly mentioned (`@smooth-brain-review`).
4. **Documented in-file:** a YAML comment in `SKILL.md` explains why the flag is set and when to remove it. Remove it when the skill is used outside OpenCode (the flag may be unsupported or ignored) or when natural-language triggering is wanted. If it is removed, keep the "never self-invoke" guard and the cost gate ([ADR 0006](0006-cost-gate-and-git-repo-check.md)).

## Alternatives Considered

| Option | Why not |
|---|---|
| Keep it as a working-style behavior only | Not repeatable, no isolation from conversation context, and no structure for the output. |
| Name it just "Smooth Brain" | Collides with the existing `troubleshoot` slow-mode trigger, and does not say what the skill does. |
| Allow model-invoked triggering | A model could start a token-heavy run without being asked. A guard line in the skill body alone is weaker than not exposing the skill at all. |

## Consequences

- The skill cannot be found by natural-language matching in OpenCode. The user must type `@smooth-brain-review`.
- Users on other harnesses must remove the flag and rely on the in-file guard.
- The word "review" is required in any phrase trigger.
