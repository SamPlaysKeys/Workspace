---
type: ADR
Status: Accepted
Date: 2026-11-11
---

# ADR 0005: Stay read-only; the report goes to chat, and a stated path approves the write

## Context

A review skill that edits code, or silently creates files, defeats its own purpose and surprises users. At the same time, people often want the result saved, and asking "may I write this?" after a long, costly run is friction when they already said where.

## Decision

1. **Never write to the target.** The skill reports findings with at most a one-line direction hint. It fixes nothing and acts on nothing unrelated; stray issues are reported as findings.
2. **Chat is the default output.** A fixed report structure: scope and roster, what the code actually does, intent vs reality, confirmed findings, contested/unverified, rejected, closing.
3. **One optional file.** The only permitted write is a single review file, written once by the main agent from `templates/review.md`. Delegates and verifiers never write.
4. **Output path as approval.**
   - A path in the trigger ("save the output to X") is both the location and the approval. The user is not asked again at the end.
   - A path ending in `.md` is used exactly as written. Any other path is treated as a directory and gets `YYYY-MM-DD-<slug>.md`.
   - `~` is expanded and missing parent folders are created.
   - An existing file is never overwritten; a numeric suffix is added.
   - With no path, the report stays in chat and the closing line offers to write to `docs/reviews/` (the default location).
5. **Template fidelity.** The file is filled from the template. New sections are not invented.
6. **Verifier execution is the one exception** to "nothing runs", and it is confined to a temp directory ([ADR 0004](0004-adjudication-wave-and-severity.md)).

## Alternatives Considered

| Option | Why not |
|---|---|
| Always write a file | Clutters repos, and many reviews are read once. |
| Always ask before writing | Redundant when the user already gave a path. |
| Let the skill fix findings | Mixes review and change, removes the review's independence, and widens blast radius. |
| Overwrite on re-run | Loses earlier reviews; a numeric suffix is cheap. |

## Consequences

- Reviewing is safe to run against any target the user can read.
- The chat report is the primary artifact, so it must stand on its own.
- Saved reviews accumulate under a predictable path and template, which makes them easy to find and compare.
