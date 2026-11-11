# Smooth Brain Review

A manual-only, read-only "fresh eyes" code review. It convenes a congress of context-free subagents, each with a different lens. It then tries to refute their findings and reconciles what the code actually does against what we believe it does.

The full procedure is in [`SKILL.md`](SKILL.md). The report template is in [`templates/review.md`](templates/review.md).

## Usage
- `@smooth-brain-review` followed by a target (any code by default, or a diff, branch or PR).
- Add `quick` to run 3 delegates instead of 6.
- Add "save the output to `<path>`" to also write the review file. A `.md` path is used as written. A directory gets `YYYY-MM-DD-<slug>.md`. Without a path the review is chat only, with an offer to write to `docs/reviews/`.
- The skill prints a cost gate (target, roster, estimated subagent runs, resolved output path) and waits for "go" before launching anything.

The skill sets `disable-model-invocation: true`, so it loads only when you mention it explicitly. Remove that line to use it outside OpenCode or to allow natural-language triggering. See the comment in `SKILL.md`.

## Choosing the subagent and model (cost saving)
Delegates and verifiers use the `explore` subagent by default. You can name a different subagent in your request, and you can name a model for it as well. This is mainly useful for cutting cost, since a full run is 6 delegates plus up to 8 verifiers.

Examples:
- "`@smooth-brain-review` `src/auth/` using the `explore-fast` subagent"
- "`@smooth-brain-review` `src/auth/` with a cheaper model, e.g. `<provider>/<model>`"

Notes:
- Look up the exact model ID with the models tool (`providerID/modelID`). Do not guess it.
- The subagent must be read-only. Verifiers need to run shell commands, so a subagent that cannot run them can only check claims by reading code.
- The choice applies to both the delegates and the verifiers.
- A weaker model may make more mistakes, and the adjudication wave catches only some of them.
- The chosen subagent and model appear in the cost gate for you to confirm.

## What it will not do
- It never edits the code under review.
- It never writes anywhere except the single review file, and only when you approved a location.
- Verifiers may run code to test a claim, but only in a temp directory and never inside the target.
