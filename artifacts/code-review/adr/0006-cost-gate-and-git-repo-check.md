---
type: ADR
Status: Accepted
Date: 2026-11-11
---

# ADR 0006: Confirm before launch, and check that the target is a git repo

## Context

A full run spends up to 14 subagent runs and sends the target's contents to a model provider. Two things need to be visible before that happens: the size of the spend, and what exactly is about to be read.

During testing the skill was pointed at a directory that turned out not to be a source repository. It was an OpenShift installer working directory: 28 files and about 10 MB of generated state, logs, credentials, kubeconfigs and TLS keys, with one small hand-written input file. Launching would have sent live secrets and megabytes of logs to the model provider. The problem was only noticed because the agent looked at the directory first. There was no guard in the skill.

## Decision

1. **Cost gate, always.** Before launching anything, the skill prints a plan and waits for an explicit "go":
   - target, repo status, roster (6 delegates, or 3 for `quick`);
   - the adjudication wave and the estimated number of subagent runs (about 6 + up to 8);
   - the subagent type and model (default or requested);
   - the **resolved** output path, or "chat only", so typos are caught before tokens are spent.
   One "go" approves both the launch and the file write.
2. **Git-repo pre-gate check.** Before printing the plan, run the read-only `git -C <target dir> rev-parse --is-inside-work-tree`.
   - **Inside a repo:** continue, and show the repo root and branch in the plan.
   - **Not a repo:** do not launch. Flag it in the plan and ask the user to confirm the target is what they intended. Proceed only on an explicit "go" that acknowledges it. Delegates then list the layout with `ls` or `find` instead of `git ls-files`.
   - Diff, branch and PR targets are git by definition and skip the check.
3. **The check does not scan contents.** It is a cheap prompt to confirm, not a secret scanner.

## Alternatives Considered

| Option | Why not |
|---|---|
| Content scanning for secrets before launch | Needs a scanner and judgment calls about what counts, and it reads the files it is trying to protect. Git-repo status catches the common cases (generated output, backups, scratch areas) at near-zero cost. |
| Refuse any non-git target | Plenty of legitimate targets are not repos (a downloaded tarball, a vendored folder). Confirming is enough. |
| No gate, trust the user's command | The estimate and the resolved output path are the point. A "go" step costs one message. |
| Gate only when the target is large | Size alone does not tell the user what is being read. |

## Consequences

- Every run costs one extra message before it starts.
- A repo check catches most "wrong directory" cases, but not secrets committed inside a git repo. Users are still responsible for that.
- For non-git targets, reviewers lose `git ls-files` and fall back to listing.
