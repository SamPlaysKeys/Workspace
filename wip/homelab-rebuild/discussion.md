# Homelab Rebuild — Discussion

## 2026-05-04 — GRADUATED

Storm session started and graduated in same day. Documentation moved to `docs/homelab/`.

**Graduated artifacts:**
- `docs/homelab/README.md` — Overview and entry point
- `docs/homelab/architecture.md` — Diagrams and environment map
- `docs/homelab/environments.md` — Detailed environment breakdown
- `docs/homelab/decisions.md` — Key decisions with rationale
- `docs/homelab/repo-structure.md` — Infrastructure repo layout
- `docs/homelab/roadmap.md` — Open questions and future work

**Remaining in wip/:**
- `scratch/komodo-plex/` — Example Plex configs (Komodo and ArgoCD for comparison)
- `scratch/argocd-plex/` — K8s manifests for comparison

---

### Session Log

Storm session started. Greenfield rebuild — starting from scratch.

### Logical Divisions

| Environment | Platform | Management |
|-------------|----------|------------|
| **Prod** | Unraid + MiniPC (Docker containers) | GitOps |
| **Test** | Synology NAS + MiniPC (Docker containers) | GitOps |
| **DevOCP** | OpenShift on 3× MiniPC cluster | Git-aware, not GitOps |
| **DevDocker** | Docker w/ Arcane (or similar) on ProxMox (server) | Unmanaged, can pull from Git |
| **DevNode** | ProxMox VMs | Unmanaged |

**Pattern:** Prod/Test are GitOps-managed. Dev environments are progressively less managed — DevOCP pulls from Git, DevDocker/DevNode are sandboxes.

### Environment Purposes (clarified)

| Environment | Purpose | Promotion | Komodo Managed |
|-------------|---------|-----------|----------------|
| **Prod** | Production workloads | — | Yes |
| **Test** | Pre-prod validation | → Prod | Yes |
| **DevDocker** | Container dev/testing | → Test → Prod | Yes |
| **DevNode** | VM testing (bugfixes, themes, etc.) | None — isolated | No |
| **DevOCP** | OpenShift work projects | None — isolated (work-related) | No |

**Key insight:** Three Komodo-managed environments (Prod, Test, Dev) with promotion path Dev → Test → Prod. DevNode and DevOCP are unmanaged sandboxes.

### Hardware

- **MiniPCs**: Multiple — used for Prod, Test, and DevOCP cluster (3× for OCP)
- **Server**: Consumer-grade PC with significant compute/memory — runs ProxMox for DevDocker and DevNode
- **NAS**: Synology — part of Test environment

### GitOps Tooling — TBD

**Current state:** GitHub Actions → Ansible playbooks running locally to deploy/update containers. Works but not sustainable.

**Considerations:**
- ArgoCD — familiar with it, but oriented toward Kubernetes. Unclear if useful for Docker containers.
- Arcane — Docker management UI (like Portainer). Modern UI, real-time monitoring. Not GitOps-native from initial look.
- Other options to evaluate?

**Open question:** What's the right GitOps approach for Docker container management (not k8s)?

### Decision: Komodo for Container GitOps

After comparing Komodo vs ArgoCD/K8s, decision is to use **Komodo**.

**Rationale:**
- ~2.5x less config complexity than K8s path
- Uses familiar Docker Compose concepts
- Avoids K8s operational risk concerns for homelab prod

**Architecture (locked in):**

```
┌─────────────────────────────────────────────────────────────────┐
│                     Komodo Controller (NUC)                     │
│                         Komodo Core                             │
│            ┌──────────────┴──────────────┐                      │
│            ▼                             ▼                      │
│   ┌─────────────────┐           ┌─────────────────┐             │
│   │  Prod Nodes     │           │  Test Nodes     │             │
│   │  (Periphery)    │           │  (Periphery)    │             │
│   └─────────────────┘           └─────────────────┘             │
└─────────────────────────────────────────────────────────────────┘
         ▲                                 ▲
         │         Git (ResourceSync)      │
         └────────────────┬────────────────┘
                          │
              ┌───────────┴───────────┐
              │   infrastructure/     │
              │   └── komodo-configs/ │
              │       ├── prod/       │
              │       └── test/       │
              └───────────────────────┘
```

**Tooling Stack:**
- **Terraform**: Provision VMs, network, base infra
- **Ansible**: System config, package updates, install Komodo Periphery
- **Komodo**: Container orchestration via GitOps (ResourceSync)

**Key patterns:**
- Stacks (compose files) for grouped apps (e.g., *arr suite)
- Individual TOML files for single apps
- Promotion = move/copy TOML between `test/` and `prod/`
- `deploy: true` flag for auto-deployment on Git changes

See: `scratch/komodo-ideas/repo-concept.md` for full repo structure.

---
