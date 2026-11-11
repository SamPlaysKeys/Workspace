---
type: README-Note
---

# Code Review: Smooth Brain Review skill

A portable Agent Skill for a read-only, "fresh eyes" code review. It convenes a congress of context-free subagents, each with a different lens. It then tries to refute their findings and reconciles what the code actually does against what was believed about it.

---

## Directory Structure

```text
artifacts/code-review/
├── README.md                          # This guide
├── adr/                               # Why the skill works the way it does (7 ADRs)
└── skills/
    └── smooth-brain-review/
        ├── SKILL.md                   # The skill: 7-step procedure, lenses, rules
        ├── README.md                  # Usage, subagent/model choice, guarantees
        └── templates/review.md        # Template for the optional review file
```

---

## What it does

1. Parses the target (any code, or a diff, branch or PR), the mode (`full` or `quick`), an optional output path, and an optional subagent/model.
2. Checks the target is in a git repo (or asks you to confirm), then shows a cost gate and waits for "go".
3. Launches 6 context-free delegates in parallel (3 in `quick`). The Smooth Brain delegate does a plain-language blind read.
4. Verifies the findings with fresh refutation-minded verifiers (which may run code in a temp directory), then re-rates severity.
5. Reconciles the blind read against stated intent, and reports in chat. It optionally writes one review file from the template.

It never edits the code under review. See the [ADRs](adr/README.md) for the reasoning behind each choice.

---

## Install

1. Copy `skills/smooth-brain-review/` into your harness's skills folder (for OpenCode: `.agents/skills/` in a project, or `~/.config/opencode/skills/` globally).
2. Invoke it with `@smooth-brain-review <target>`.

**Before you install, read these notes:**
- `SKILL.md` sets `disable-model-invocation: true` so the skill only loads when mentioned explicitly ([ADR 0001](adr/0001-name-and-manual-only-invocation.md)). Remove that line if your harness does not support the flag, or if you want natural-language triggering. Keep the "never self-invoke" guard and the cost gate if you do.
- The skill delegates to a subagent type named `explore` (read-only exploration). If your harness names it differently, change it in `SKILL.md` (Steps 3 and 4). See [ADR 0007](adr/0007-skill-as-orchestrator-no-custom-agent.md).
- The skill needs a harness that can spawn subagents in parallel. Verifiers need shell access to run code; without it, they can only check claims by reading.
- A full run can use up to about 14 subagent runs. You can pick a cheaper subagent or model per run (see the skill's README).

## Source of truth

This is a copy of the skill. If you edit one copy, port the change to the other. In the workspace this was packaged from, the live copy is `.agents/skills/smooth-brain-review/`.

## Prerequisites

- **Git**, for the repo check and for diff, branch and PR targets.
- **GitHub CLI (`gh`)**, only for PR targets.
