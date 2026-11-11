---
type: ADR
Status: Accepted
Date: 2026-11-11
---

# ADR 0004: Verify findings with an adjudication wave, then re-rate severity

## Context

Delegates are cheap to run and prone to guessing. In the first dry run most findings were marked `inferred`, delegates disagreed on severity, and one lens rated "no tests in a 30-line plugin" as High. Several runtime claims were pure speculation until someone actually ran the code. Verification found, for example, that a missing clipboard binary makes the code throw, which no delegate had tested.

## Decision

After the delegates report, the main agent clusters and dedupes findings, records which lenses agreed and where they disagree, then runs an **adjudication wave** by default.

1. **What gets verified.** A fresh, context-free verifier tries to **refute** each of:
   - every High or Critical finding;
   - every single-lens finding;
   - every convergent finding where *all* agreeing lenses marked it `inferred` or `assumed`, because agreement between guesses is not evidence;
   - any finding where delegates disagree on severity by two or more levels.
2. **What a verifier receives.** Only the claim, the cited location, and how to access the target. It must cite evidence to refute.
3. **Verifiers may run code** to test a claim, under strict rules:
   - run only inside a temp directory (the environment's designated one, else a fresh `mktemp -d`);
   - never create, edit or delete anything in the target, and leave no build artifacts, caches or lockfiles there;
   - quote or escape paths containing spaces;
   - report the exact command and output as evidence.
4. **Outcomes.** *Confirmed* survived refutation. *Refuted* is dropped but counted. *Inconclusive* moves to "Contested / unverified".
5. **Cap.** At most 8 verifiers. If more findings qualify, batch by file.
6. **Severity re-rating.** After adjudication the **main agent** sets the final severity, judging real-world impact, likelihood, and the size and nature of the target. Changes from the delegates' rating are noted with a short reason (for example "High → Low: 30-line plugin").

## Alternatives Considered

| Option | Why not |
|---|---|
| Trust delegate output as is | Inflated severities and unverified guesses reach the report. |
| Verify every finding | Cost grows with finding count, and most Low findings are cheap to accept. The criteria above target where wrong findings are most likely or most costly. |
| Verifiers read code only, never run it | Runtime behavior (throws, silent failures, symptoms of missing dependencies) cannot be settled by reading. |
| Let verifiers run anywhere | Violates the read-only guarantee on the target ([ADR 0005](0005-read-only-and-output-path-as-approval.md)). |
| Average the delegates' severities | Averages inherit the inflation. A judgment by the agent with the whole picture is more reliable. |

## Consequences

- Confirmed findings are backed by evidence, often a reproduced command.
- Adjudication adds up to 8 more subagent runs. This is part of the cost shown at the gate ([ADR 0006](0006-cost-gate-and-git-repo-check.md)).
- The skill's "read-only" promise is about the *target*. Verifiers do execute code, so users should be aware of that, particularly for untrusted code.
