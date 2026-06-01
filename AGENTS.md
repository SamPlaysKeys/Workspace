---
type: Note
_organized: true
---

# AGENTS.md — Workspace Conventions

This workspace follows specific co-creation conventions defined in [[working_style]]. Agents MUST adhere to these behaviors and conventions.

## Core Behaviors
- **Storm Session**: Rapid ideation in `wip/`. Disposable by design.
- **Pre-Mortem**: Adversarial review before significant decisions. Proactively suggest this.
- **Smooth Brain**: Question assumptions at session start or when stuck.
- **Progressive Bookkeeping**: Keep state files (`context.md`, `BACKLOG.md`, `ACTIVITY.md`) current throughout the session.
- **Dead Drop**: Structured handoff in `planning/whats-next.md` at session end.
- **Troubleshoot**: Structured investigation and remediation documentation.
- **Sneaky**: Use `.workspace/` for working files in external repos.

## Core Conventions
- **Isolation**: Stay within this project unless cross-repo work is explicitly requested.
- **Append, Don't Replace**: Accumulate entries in state/log files to preserve history.
- **Documentation Structure**: Follow the established `docs/` hierarchy for graduated work.

Refer to [[working_style]] for detailed implementation guidance for each behavior.

## What agents should do
- Update `AGENTS.md` only when the user asks for workspace-level guidance changes.
- Follow the co-creation conventions for all interactive sessions.
- Do not assume the workspace is for a specific project (like a homelab or a Tolaria vault) unless explicitly requested.