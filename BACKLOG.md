# Backlog

Human-in-the-loop review and tracking of what has been done and what is in progress.

> **Purpose**: This file is for high-level tracking of work items that benefit from human review and prioritization. It's the place to capture ideas, track progress on larger initiatives, and mark things complete. Unlike automated task tracking, this is intentionally manual to keep a human in the loop.

---

## In Progress

- [ ] Populate `artifacts/` with reusable automation components
- [ ] Create initial skills in `workstyle/skills/`
- [ ] Set up pre-commit hooks for secret scanning

---

## Completed

- [x] Initial repository structure planning
- [x] Reorganize directory structure to match revised plan
- [x] Establish `planning/` directory for workstream management
- [x] Codify purpose of tracking files (BACKLOG, ACTIVITY, whats-next)
- [x] Define co-creation behaviors in `workstyle/working_style.md` (Storm Session, Pre-Mortem, Smooth Brain, Progressive Bookkeeping, Dead Drop)
- [x] Create `wip/` directory for Storm Sessions
- [x] Create Cursor rule for co-creation behaviors (`.cursor/rules/co-creation.mdc`)
- [x] Add conventions to working_style.md (Isolation, Append-Don't-Replace)
- [x] Add Close-out compound behavior under Dead Drop
- [x] Add Troubleshoot behavior for structured debugging and knowledge capture

---

## Ideas / Future

*Capture ideas here before promoting them to "In Progress"*

- [ ] Tags/categories for troubleshooting docs findability — as `docs/troubleshooting/` grows, may need metadata or naming conventions to help locate past fixes by symptom, system, or error type
- [ ] Auto-load working_style for CLI agents — explore how to automatically load `workstyle/working_style.md` when launching CLI agents (Cursor, Gemini, OpenCode, Copilot) in this repo; consider AGENTS.md, CLAUDE.md, repo-level configs, wrapper scripts
