# Homelab Rebuild — Context

**Goal:** Plan a greenfield rebuild of the homelab with clear environment separation and GitOps management for prod/test.

**Status:** GRADUATED — Documentation moved to `docs/homelab/`

**Current state:** Core architecture decided. Tooling stack selected. Documentation created.

**Decided:**
- **GitOps tooling:** Komodo (not K8s/ArgoCD)
- **Infra management:** Terraform + Ansible
- **Container management:** Komodo Core (on dedicated NUC) + Periphery agents on nodes
- **Promotion model:** Move/copy TOML files between environment directories
- **Repo structure:** See `scratch/komodo-ideas/repo-concept.md`

**Open questions:**
- Specific workloads to run in Prod/Test?
- Network topology between environments?
- Storage strategy (Unraid ↔ Synology roles)?
- Terraform + Ansible boundary — what does each manage exactly?
- Dev environments (DevOCP, DevDocker, DevNode) — how do they fit with Komodo?
- Budget and timeline?

**Key constraints:**
- Prod/Test/Dev (DevDocker) are GitOps-managed via Komodo
- DevOCP is isolated (work-related OpenShift projects) — unmanaged
- DevNode VMs on ProxMox are sandboxes — unmanaged
- Komodo Controller is dedicated hardware (NUC) — no workloads, only management
