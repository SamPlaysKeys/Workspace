# Activity Log

Daily activity summaries and significant changes to the workspace.

---

## 2026-04-22

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
- Added `docs/BACKLOG.md` for task tracking
- Added `docs/ACTIVITY.md` (this file) for activity logging
- Updated `planning/README.md` to reflect the new structure

**Rationale:**
Moving rules and skills under `workstyle/` consolidates AI behavior configuration in one place. The `artifacts/` directory provides organized storage for reusable automation components (Ansible roles, Bash scripts, GitHub Actions, OpenShift manifests).
