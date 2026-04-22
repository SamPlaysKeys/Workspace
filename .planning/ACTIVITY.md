# Activity Log

Tracking **meta changes** to the workspace structure, conventions, and planning systems.

> **Purpose**: This file records structural and organizational changes to the workspace itself—not project work, but changes to how work is organized, tracked, and documented. Think of it as a changelog for the workspace's operating system.

---

## 2026-04-22

### Planning System Refinement

Restructured the planning and tracking system to clarify file purposes and locations.

**Changes made:**
- Created `.planning/` directory as central point for active task planning
- Moved `BACKLOG.md` from `docs/` to repository root
- Moved `ACTIVITY.md` from `docs/` to `.planning/`
- Created `.planning/whats-next.md` for tracking active tasks and commits
- Added multi-project workstream management as a core goal in `roadmap/planning.md`
- Updated `workstyle/README.md` with progressive tracking/documentation principles

**File purposes codified:**
- `BACKLOG.md` (root): Human-in-the-loop review and tracking of completed/in-progress work
- `.planning/ACTIVITY.md`: Meta changes to workspace structure and conventions
- `.planning/whats-next.md`: Active task tracking with associated commit hashes

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
