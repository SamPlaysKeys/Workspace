# Komodo Artifact Templates

**Goal**: Create reusable TOML templates for Komodo configuration that encapsulate common patterns (CI/CD, service definition, scheduled jobs, etc.). These templates will populate `artifacts/komodo/` for reuse across deployments.

**Current state**: 
- `artifacts/` exists with some categories (Ansible, bash, GitHub Actions, OpenShift)
- No Komodo-specific artifacts yet
- Need to align with Komodo's declarative, GitOps-based approach (pull model)

**Open questions**:
- What are the most common/reusable Komodo configuration patterns?
- Should we include example `komodo.yml` (global) alongside service-specific TOMLs?
- How to structure the artifacts to support composability?

**Key constraints**:
- TOML files must be syntactically valid
- Must align with Komodo's expected schema
- Should be flexible enough to customize for specific environments
- Include examples for validation, secrets handling, and scheduling