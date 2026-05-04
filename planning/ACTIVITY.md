# Activity Log

Tracking **meta changes** to the workspace structure, conventions, and planning systems.

> **Purpose**: This file records structural and organizational changes to the workspace itself—not project work, but changes to how work is organized, tracked, and documented. Think of it as a changelog for the workspace's operating system.

---

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
  - Remediation doc (always) → `docs/troubleshooting/<system>/`
  - Prevention/setup doc (conditional) → `docs/guides/<system>/`
- Quick doc path for known fixes (skip session, write directly to docs/)
- Agent prompts for prevention doc if unsure, never skips without user confirmation

**Files updated:**
- `workstyle/working_style.md` — Added Troubleshoot behavior section
- `.cursor/rules/co-creation.mdc` — Added Troubleshoot to behaviors table and key points

**Source:**
Workshopped in Storm Session `wip/workspace-behaviors/`
