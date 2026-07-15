---
type: Note
_organized: true
---

# AGENTS.md — Workspace Conventions

This workspace enforces strict alignment and controlled execution. Do not run off and do work before the human is ready.

## Rules of Engagement

- **Alignment Checks (Checkpoints):** Before executing destructive bash commands, modifying more than 2 files, or making architectural decisions, you MUST present a brief plan and ask: *"Does this match what you had in mind?"* Wait for explicit approval before proceeding.
- **No Unprompted Sweeps:** Fix *only* what was requested. If you notice unrelated tech debt, typos, or other bugs while working, surface them in chat for later. Do not include them in your current edits.
- **Progressive Bookkeeping:** Update state files (`BACKLOG.md`, `context.md`, etc.) as progress happens, not just at the end of a session.
- **Isolation:** Stay within this project unless cross-repo work is explicitly requested. Do not modify external projects.
- **Append, Don't Replace:** State and log files should accumulate entries to preserve history.
- **No nginx Unless Required:** When designing service delivery, default to direct HTTP access via Tailscale Serve (pointing at the service port). Do not propose nginx or any reverse proxy unless the service requires TLS termination, path-based routing, or virtual hosting that Tailscale Serve cannot provide.

## Interactive Workflows (Skills)

Complex or structured workflows are handled by dedicated Skills that the agent will auto-load based on your natural language:
- **Ideation (Storm Sessions):** Quarantines exploratory work to the `wip/` directory. (Trigger: "start a storm sesh")
- **Troubleshoot:** Phased debugging and investigation. (Trigger: "troubleshoot this")
- **Handoff (Dead Drop):** Captures session state. (Trigger: "drop a breadcrumb")
- **Document:** Generates standardized guides and remediation docs. (Trigger: "write this up")

## What agents should do
- Update `AGENTS.md` only when the user asks for workspace-level guidance changes.
- **Maintain Portability:** If a core behavior or skill is updated in `.agents/skills/` or `AGENTS.md`, you MUST mirror that update in the portable `workstyle/working_style.md` document so it remains perfectly in sync.
- Follow these rules for all interactive sessions.
- Do not assume the workspace is for a specific project unless explicitly requested.