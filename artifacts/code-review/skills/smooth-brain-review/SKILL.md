---
name: smooth-brain-review
description: Manual-only, read-only "fresh eyes" code review. Convenes a congress of context-free subagents (multiple viewpoints), cross-examines their findings, and reconciles what the code actually does against stated intent. Invoke only via explicit `@smooth-brain-review`.
# NOTE: `disable-model-invocation: true` hides this skill from the model's skill list,
# so it loads ONLY when explicitly mentioned (e.g. `@smooth-brain-review`). This is
# intentional: the skill spends a lot of tokens and must never fire on its own.
# Remove this line if you (a) use this skill outside OpenCode, where the flag may be
# unsupported or ignored, or (b) want natural-language triggering such as
# "let's do a smooth brain review of x". If you remove it, keep the strict
# "never self-invoke" guard below and the cost gate in Step 2.
disable-model-invocation: true
---

# Smooth Brain Review

<objective>
Review code without assumptions. Separate what the code **does** from what we **believe** it does. Use a congress of context-free subagents, each with a different viewpoint, so no single reader's bias or the conversation's prior beliefs can anchor the result.
</objective>

## Guard (read first)
- **Never self-invoke and never suggest this skill unprompted.** It runs only on an explicit user request.
- **Read-only.** Never write to the target. The only permitted write is the single review file in Step 7, and only when an output location was requested or confirmed. (Verifiers may run code, but only under the temp-directory rule in Step 4.)
- **Fix nothing.** Findings may carry a one-line "direction" hint; do not edit reviewed code.
- Stay in scope. Surface unrelated issues as findings; never act on them.

## Triggers
Manual only. Examples:
- `@smooth-brain-review` / "let's do a smooth brain review of `<target>`"
- "... and save the output to `<path>`" (see Output request)
- Modifier: `quick` (3 delegates instead of 6)

Because "smooth brain" is also a slow-mode trigger in `troubleshoot`, this skill requires the word **review**.

## Step 1 — Parse the request
Extract three things from the user's message:

1. **Target**
   - Default: "any code" — the files, directories, or area the user names. If none is named and the area is ambiguous, ask once.
   - Directed forms: a diff (`git diff A...B`), a branch (vs. its merge base), or a PR (`gh pr diff <n>`).
2. **Output request** (optional): a phrase like "save to / write to / output to `<path>`".
   - A path ending in `.md` is used exactly as written.
   - Any other path is a directory: write `<dir>/YYYY-MM-DD-<slug>.md`.
   - Expand `~`. Create missing parent folders. Never silently overwrite: if the file exists, add a numeric suffix.
   - **A stated output path is the approval for the write.** Do not ask again at the end.
   - No path given: chat only, with an end-of-report offer to write to `docs/reviews/` (default location).
3. **Mode**: `full` (default) or `quick`.
4. **Subagent / model** (optional): the user may name a subagent type and/or a model for the delegates and verifiers (e.g. for cost saving). Default is the `explore` subagent with its default model. Pass a model only if the user names one, and resolve its exact `providerID/modelID` with the models tool; never guess it. The subagent must be read-only.

### Reset declaration
Write down (privately, for Step 5) the beliefs being set aside: stated intent, PR title/description, commit messages, earlier conclusions in this conversation, and any "this was fixed / tested / unused" claims. **None of this goes to any delegate or verifier.**

## Step 2 — Cost gate (always confirm before launch)
### Pre-gate check: is the target in a git repo?
Before printing the plan, run `git -C <target dir> rev-parse --is-inside-work-tree` (read-only). Diff, branch, and PR targets are git by definition and skip this check.
- **Inside a repo:** continue. Note the repo root and branch for the plan.
- **Not a repo:** do not launch. Say so plainly in the plan, and ask the user to confirm the target is what they intended (a non-repo directory is often generated output, a backup, or a scratch area that may hold secrets or logs). Proceed only on an explicit "go" that acknowledges this. Delegates then list layout with `ls`/`find` instead of `git ls-files`.

This check does not scan contents. It is only a prompt to confirm.

Resolve the target and print a short plan, then wait for an explicit "go":

```
Smooth Brain Review
Target:   <resolved target: paths, or diff/branch/PR refs>
Repo:     <git repo root @ branch | NOT A GIT REPO — confirm target>
Roster:   6 delegates (Smooth Brain, Skeptic, Saboteur, Gatekeeper, Operator, Maintainer)
          [quick: 3 — Smooth Brain, Skeptic, Saboteur]
Then:     adjudication wave (~1 verifier per High/Critical or single-lens finding, cap 8)
Estimate: ~6 + up to 8 subagent runs
Agent:    <subagent type, and model if one was requested | explore, default model>
Output:   <resolved file path | chat only>
Proceed?
```

Echo the **resolved** output path so typos are caught before tokens are spent. One "go" approves both the launch and the file write.

## Step 3 — Convene the Congress
Spawn all delegates **in parallel** using the `explore` subagent (read-only by construction), or the subagent/model the user requested in Step 1. Each delegate starts with a fresh context. Verifiers in Step 4 use the same subagent and model.

### Neutral brief (identical for every delegate)
Contains only:
- The target: file/directory paths, or the exact read-only command to view the diff (`git diff A...B`, `gh pr diff <n>`). If a delegate cannot run commands, inline the diff instead, chunked by file.
- The output schema (below).
- The rules (below).
- That delegate's lens charter (below).
- A note on paths: quote or escape any path containing spaces in commands (e.g. `"/path/Personal Projects/x"`, or `\ ` inside `set rtp+=`).

It must **not** contain: intent, PR/commit text, ticket links, prior findings, or anything from this conversation.

### Repo layout vs. withheld material
Delegates **may list the repo layout** (`git ls-files`, `ls`, `find`, glob) so they can tell whether files such as `plugin/`, tests, CI, or config exist. Listing names is allowed; **reading the withheld material is not.** State this explicitly in the brief:
- Do not open or read README files, CHANGELOGs, docs, help files, prior reviews, or any other file that states intent. Their names may appear in a listing; do not read them.
- Do not read commit messages or PR text (`git log`, `git show`, `gh pr view`).
- Read only the target files and what is needed to confirm what exists around them.

### Rules for every delegate
- Do not assume intent. If behavior looks deliberate, report it as **undocumented behavior**; do not excuse it.
- Verify before claiming. Cite `file:line`. Mark each claim *verified* (read the line / ran a read-only command), *inferred*, or *assumed*.
- Read-only: never edit, create, or delete files.
- Report what you could not verify.
- Do not stop at the first finding.

### Output schema (each finding)
`severity (Critical|High|Medium|Low) | file:line | claim | evidence | confidence (verified|inferred|assumed) | could-not-verify`

### Lens charters
| Delegate | Charter |
|---|---|
| **Smooth Brain** | Plain-language only: what does this code actually do, step by step? No judgment, no intent, no fixes. Output a short summary plus a list of anything you could not explain. This is the blind read. |
| **Skeptic** | Logic and correctness: edge-case inputs, off-by-ones, wrong conditions, type/null surprises, wrong assumptions baked into branches. |
| **Saboteur** | Failure paths: errors, partial failure, retries, timeouts, idempotency, "what if this runs twice, or never, or half-way". |
| **Gatekeeper** | Security and trust boundaries: untrusted input, injection, secrets, permissions, data exposure. |
| **Operator** | Hidden dependencies: environment, ordering, shared state, config, other files, deploy-time and runtime surprises. |
| **Maintainer** | Newcomer's view: readability, naming, "what would I be afraid to change", and omissions (missing tests, docs, handling). |

`quick` runs Smooth Brain, Skeptic, and Saboteur only. **Smooth Brain is mandatory in every mode.**

## Step 4 — Deliberation
Main agent processes the delegate output:
1. **Cluster and dedupe** findings by location and claim.
2. **Convergence:** a finding raised by 2+ lenses gains confidence; record which lenses.
3. **Contested:** where lenses disagree, keep both positions side by side.
4. **Adjudication wave:** spawn fresh, context-free `explore` verifiers (parallel) to try to **refute** each of these:
   - every High/Critical finding;
   - every single-lens finding;
   - every **convergent finding where all agreeing lenses marked it `inferred` or `assumed`** (agreement between guesses is not evidence); and
   - any finding where the delegates disagree on severity by two or more levels.

   A verifier gets only the claim, the cited location, and the target access method, and must cite code evidence to refute.
   - **Verifiers may run code** to test a claim (e.g. a headless runtime, a script, a test), under these rules:
     - Run only inside a temp directory (the environment's designated temp dir if one is given, otherwise a fresh `mktemp -d`). Copy or reference target files; do not run anything that writes into the target.
     - Never create, edit, or delete files in the target, and leave no build artifacts, caches, or lockfiles there.
     - Quote or escape paths containing spaces.
     - Report the exact command and output as evidence.
   - **Confirmed** = survived refutation. **Refuted** = dropped, but counted. **Inconclusive** = moved to "Contested / unverified".
   - Cap at 8 verifiers; if more findings qualify, batch by file.
5. **Severity re-rating (main agent):** delegates tend to inflate severity (e.g. "no tests" as High). After adjudication, set the final severity for each confirmed finding yourself, judging real-world impact, likelihood, and the size and nature of the target. Where it differs from the delegates' rating, keep the final rating and note the change and reason briefly (e.g. "High → Low: 30-line plugin"). Use the final rating in the report and file.

## Step 5 — Reconcile
Compare the Smooth Brain summary against the beliefs set aside in the reset declaration (stated intent, PR text, prior claims). Report each mismatch: what was believed, what the code does, evidence.

## Step 6 — Chat report
Fixed structure:
1. **Scope & roster** (target, mode, delegates, verifier count)
2. **What this actually does** (Smooth Brain summary)
3. **Intent vs reality** (Step 5 mismatches)
4. **Confirmed findings** — by severity; each with `file:line`, evidence, lenses that agreed, one-line direction hint
5. **Contested / unverified**
6. **Rejected** (count only, plus a one-line reason pattern if useful)
7. **Closing:** if no output path was given, ask: *"Want this written up to `docs/reviews/`?"* If one was given, state the path written (Step 7).

## Step 7 — Write the review file (only if approved)
Approved means: an output path was given in the trigger (Step 1), or the user answers yes to the closing prompt.

1. Read `templates/review.md` (next to this file) and fill it from the report. Do not invent new sections.
2. Resolve the path per Step 1 (default for the closing-prompt case: `docs/reviews/YYYY-MM-DD-<slug>.md`).
3. Only the main agent writes, once. Delegates and verifiers never write.
4. Confirm the final path in chat.
