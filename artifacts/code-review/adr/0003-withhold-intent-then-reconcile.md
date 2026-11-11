---
type: ADR
Status: Accepted
Date: 2026-11-11
---

# ADR 0003: Withhold intent from reviewers, then reconcile against it

## Context

If reviewers are told what the code is supposed to do, they tend to confirm it. The skill's value comes from comparing two independent things: what the code actually does, and what was believed or stated about it (README, changelog, PR text, commit messages, earlier conclusions in the conversation).

A first dry run withheld the docs from reviewers and then compared the blind read against them. It surfaced real mismatches, for example documentation implying graceful degradation when the code could throw, and a stated requirement lower than the real one. It also showed a cost: because the brief forbade reading any other file, several delegates reported "could not verify whether a `plugin/` directory or tests exist."

## Decision

1. **Reset declaration.** Before launching anything, the main agent writes down (privately) the beliefs being set aside: stated intent, PR title and description, commit messages, earlier conclusions, and any "this was fixed / tested / unused" claims.
2. **Neutral brief.** Delegates and verifiers receive only the target, the output schema, the rules, and their lens charter. No intent, PR or commit text, ticket links, prior findings, or conversation content.
3. **Layout listing is allowed.** Delegates may list the repo layout (`git ls-files`, `ls`, `find`, glob) to see whether tests, CI, config or entry-point files exist. They must not read README files, CHANGELOGs, docs, help files, prior reviews, commit messages or PR text.
4. **Reconcile afterwards.** The main agent compares the Smooth Brain summary with the set-aside beliefs and reports each mismatch as: what was believed, what the code does, evidence.
5. **Withheld is not wrong.** A reviewer finding that is explained by documented intent (for example "missing feature" that the docs call future work) is reclassified as by design, not discarded silently.

## Alternatives Considered

| Option | Why not |
|---|---|
| Give delegates the intent so they review "against the spec" | Anchors them, and loses the independent read that makes mismatches visible. |
| Withhold everything, including file names | Delegates can't tell what exists around the target and report guesses as unverifiable. |
| Let delegates read docs "only after" their first pass | Hard to enforce in a single-shot subagent run, and still contaminates later findings. |

## Consequences

- The reconcile step reliably finds doc/code drift.
- Delegates can say what does and doesn't exist in the repo, so fewer findings are "could not verify".
- The boundary (names yes, contents of intent files no) must be stated explicitly in every brief.
