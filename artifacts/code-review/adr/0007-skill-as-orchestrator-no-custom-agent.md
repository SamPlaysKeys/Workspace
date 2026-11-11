---
type: ADR
Status: Accepted
Date: 2026-11-11
---

# ADR 0007: Implement as a skill that orchestrates, not as a custom agent

## Context

OpenCode supports custom agents (primary agents and subagents) as well as skills. The review needs to launch many parallel subagents, reconcile their output against what the conversation believed, and optionally write one file. The question was whether this should be a skill, an agent, or a hybrid.

## Decision

1. **A skill is the orchestrator.** It runs in the main conversation, so it can build the reset declaration from the conversation, spawn delegates and verifiers, reconcile, and write the file.
2. **Delegates and verifiers use an existing read-only subagent (`explore` by default).** No custom subagent is added.
3. **The user can choose the subagent and model per run** (for example a cheaper model to cut cost). Verifiers use the same choice as delegates. A model is passed only when the user names one, and its exact `providerID/modelID` is looked up, never guessed. The choice is shown in the cost gate.
4. **A custom `sbr-delegate` subagent was considered and not added.** The dry run showed no need: delegates stayed read-only, and verifiers ran what they needed. It can be revisited if permissions or cost need enforcing rather than requesting.

## Alternatives Considered

| Option | Why not |
|---|---|
| A primary agent for review | Requires switching agents (Tab) and is OpenCode-specific, so the skill would not travel. It also loses the conversation-context reconcile step. |
| The orchestrator as a subagent | A subagent run this way cannot itself launch further subagents in this environment, so the congress could not be convened. |
| Hybrid: skill plus a custom read-only `sbr-delegate` (shell allowlist, step cap, optional cheaper model) | Adds a second file to maintain and an OpenCode dependency for no problem observed so far. This is the upgrade path if enforcement is needed. |
| Embed everything in `AGENTS.md` | Always-on instructions would make the cost and read-only rules ambient and untriggerable. |

## Consequences

- The skill is portable. It needs only a harness with subagents and a read-only exploration agent.
- Read-only behavior for delegates is by instruction and by the choice of `explore`, not by a bespoke permission set. Treat it as a strong default, not a hard guarantee, when using a different subagent type.
- Invoking the skill from a mode that restricts writes (such as a plan mode) means it cannot write the review file. Chat output still works.
- Cost control is via the gate ([ADR 0006](0006-cost-gate-and-git-repo-check.md)) and the per-run model choice, not a dedicated agent.
