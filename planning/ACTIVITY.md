# Activity Log

Tracking **meta changes** to the workspace structure, conventions, and planning systems.

> **Purpose**: This file records structural and organizational changes to the workspace itself—not project work, but changes to how work is organized, tracked, and documented. Think of it as a changelog for the workspace's operating system.

---

### VSCode Setup Guide Storm Session (Enterprise Pivot)

Pivoted the VSCode setup guide from homelab to enterprise context. Added instructions for `git-scm`, `python3`, `ansible-lint`, and Personal Access Tokens (PAT).

**Files updated:**
- `docs/guides/dev-environment/vscode.md`
- `wip/vscode-setup/context.md`
- `wip/vscode-setup/discussion.md`

**Goal:**
Provide a comprehensive setup guide suitable for enterprise devices, ensuring all core dependencies and authentication methods are covered.

---

## 2026-05-30

### Tailscale ACL Plan

Created `docs/homelab/network/tailscale-grants.md` — full grants policy document mapping 7 tags (Prod, Test, Dev, Host, App, Storage, Admin) to Tailscale Grants syntax. Covers:

- Tag inventory with composite tag examples per node type
- User groups (admin, manager, user) matching existing access tier model
- 9 ACL rule groups with descriptions (admin full access, user web-only, manager app-only, app-to-storage, host-to-storage, SSH, environment isolation, subnet routes)
- Complete ACL JSON policy
- ACL tests for policy validation
- SSH rules (admin-to-host only)
- Auto-approvers for Docktail service advertisement
- Tag assignment examples per node type
- Open questions

### Tailscale ACL → Grants Migration

Migrated `docs/homelab/network/tailscale-acls.md` to Grants syntax:
- Renamed to `tailscale-grants.md`
- Converted all `acls` entries to `grants` — removed `action` field, separated `dst` into `dst` + `ip`, dropped `:port` from destinations
- Updated title, headings, references throughout
- Updated wikilinks in `tailscale.md` (`[[tailscale-acls]]` → `[[tailscale-grants]]`)

### Tailscale Doc Audit & Fixes

Reviewed `docs/homelab/network/tailscale.md` against actual Docktail project. Found 4 issues:

- **Docktail URL** — `github.com/docktail/docktail` returned 404; corrected to `github.com/marvinvr/docktail`
- **Docktail labels** — Used old flat format (`docktail.enable`, `docktail.hostname`, `docktail.port`); updated to `docktail.service.*` prefix
- **Docktail image** — `docktail/docktail` → `marvinvr/docktail`
- **ScaleTail** — Renamed outbound-only section to "Outbound-Only Sidecar", rewrote ScaleTail section to describe actual ScaleTail project (per-service sidecar for inbound exposure), added GitHub link
- **Architecture diagram** — Fixed stale label reference (`ts.enable` → `docktail.*`)
- **Decisions section** — Split stale "ScaleTail for outbound-only" into separate decisions for ScaleTail and Outbound-Only Sidecar

Also verified working_style behaviors: Smooth Brain (auto), Progressive Bookkeeping (active), Isolation (active).

**Files changed:**
- `docs/homelab/network/tailscale.md` — 5 edits

## 2026-05-29

## 2026-04-22

### Co-Creation Behaviors Framework

Established agent-agnostic behaviors for collaborative work in `workstyle/working_style.md`.

**Behaviors defined:**
- **Storm Session**: Brainstorming in `wip/` for net-new ideas; sessions are disposable
- **Pre-Mortem**: Adversarial review before committing to decisions; agent can initiate
- **Smooth Brain**: Surface and question assumptions; auto-triggers at session start
- **Progressive Bookkeeping**: Always-on; keep state files current throughout sessions

**Files created/updated:**
- Created `workstyle/working_style.md` — agent-agnostic co-creation conventions
- Created `wip/` directory for Storm Sessions
- Created `.cursor/rules/co-creation.mdc` — Cursor rule pointing to working_style.md

**Rationale:**
The goal is guided co-creation that works across any AI agent (Cursor, Claude, ChatGPT, etc.). The `wip/` directory provides a visible staging area for brainstorming that can be watched from Neovim or other editors. Behaviors are invocable by name for consistency.

---

### Dead Drop Behavior & Conventions

Extended the co-creation framework with session handoff capabilities and foundational conventions.

**Behaviors added:**
- **Dead Drop** (alias: Bread Crumb): Structured session state capture for handoffs
- **Close-out**: Compound behavior (Smooth Brain → Dead Drop) for session endings

**Conventions added:**
- **Isolation**: Artifacts stay in current project; no write-back to external workspaces without confirmation
- **Append, Don't Replace**: State files accumulate entries; history preserved

**Files updated:**
- `workstyle/working_style.md` — Added Dead Drop, Close-out, and Conventions section
- `.cursor/rules/co-creation.mdc` — Updated to reflect new behaviors and conventions

**Source:**
Ideas drawn from hhellbusch/my-ai-workspace, workshopped in Storm Session `wip/workspace-behaviors/`

---

### Planning System Refinement

Restructured the planning and tracking system to clarify file purposes and locations.

**Changes made:**
- Created `planning/` directory as central point for active task planning
- Moved `BACKLOG.md` from `docs/` to repository root
- Moved `ACTIVITY.md` from `docs/` to `planning/`
- Created `planning/whats-next.md` for tracking active tasks and commits
- Added multi-project workstream management as a core goal in `roadmap/planning.md`
- Updated `workstyle/README.md` with progressive tracking/documentation principles

**File purposes codified:**
- `BACKLOG.md` (root): Human-in-the-loop review and tracking of completed/in-progress work
- `planning/ACTIVITY.md`: Meta changes to workspace structure and conventions
- `planning/whats-next.md`: Active task tracking with associated commit hashes

---

### Directory Structure Reorganization

Revised the repository structure to better align with long-term goals.

**Changes made:**
- Removed empty `devops/` and `scripts/` directories
- Created `artifacts/` directory with subdirectories:
  - `artifacts/ansible/`
  - `artifacts/bash/`
  - `artifacts/github-actions/`
  - `artifacts/openshift/`
- Created `workstyle/rules/` for AI system prompts and constraints
- Created `workstyle/skills/` for custom tools and workflow definitions
- Added blank `README.md` files to all empty directories

**Rationale:**
Moving rules and skills under `workstyle/` consolidates AI behavior configuration in one place. The `artifacts/` directory provides organized storage for reusable automation components (Ansible roles, Bash scripts, GitHub Actions, OpenShift manifests).

---

## 2026-04-23

### Troubleshoot Behavior

Added sixth behavior to the co-creation framework for structured troubleshooting and knowledge capture.

**Behavior added:**
- **Troubleshoot** (alias: Debug): Structured investigation sessions for debugging external systems (OpenShift, containers, operators, etc.)

**Key features:**
- Lightweight session in `wip/troubleshoot-<system>-<issue>/` with single `investigation.md` file
- Progressive logging: Symptoms → Investigation (hypothesis/tried/result) → Resolution
- Two graduation outputs:
  - [x] Create remediation doc (always) → `docs/troubleshooting/<system>/`
    - [x] Prevention/setup doc (conditional) → `docs/guides/<system>/`
  - Quick doc path for known fixes (skip session, write directly to docs/)
  - Agent prompts for prevention doc if unsure, never skips without user confirmation

  **Files updated:**
  - `workstyle/working_style.md` — Added Troubleshoot behavior section
  - `.cursor/rules/co-creation.mdc` — Added Troubleshoot to behaviors table and key points

  **Source:**
  Workshopped in Storm Session `wip/workspace-behaviors/`

  ---

  ## 2026-05-20

  ### Agent Instruction Alignment

  Updated `AGENTS.md` to formally integrate the co-creation framework and working styles.

  **Changes made:**
  - Added "Working Styles & Behaviors" section to `AGENTS.md`.
  - Explicitly mandated adherence to behaviors (Storm Session, Pre-Mortem, Smooth Brain, Progressive Bookkeeping, Dead Drop, Troubleshoot, Sneaky).
  - Codified core conventions (Isolation, Append-Don't-Replace, Documentation Structure).
  - Pointed to `[[working_style]]` as the authoritative implementation guide.

  **Rationale:**
  Ensures that all agents (Gemini, Claude, Cursor, etc.) are aware of and follow the established working styles of this workspace automatically upon loading. This completes the "Auto-load working_style for CLI agents" idea from the backlog.

