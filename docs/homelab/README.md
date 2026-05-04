# Homelab Infrastructure

This documentation covers the design, architecture, and operational strategy for a greenfield homelab rebuild.

## Overview

The homelab is organized into **five logical environments** with different purposes and management approaches:

| Environment | Purpose | Management |
|-------------|---------|------------|
| **Prod** | Production workloads | GitOps (Komodo) |
| **Test** | Pre-production validation | GitOps (Komodo) |
| **Dev** | Container development/testing | GitOps (Komodo) |
| **DevNode** | VM sandbox (bugfixes, themes) | Unmanaged |
| **DevOCP** | OpenShift projects (work-related) | Unmanaged, isolated |

Three environments (Prod, Test, Dev) form a **promotion path** managed by Komodo. Two environments (DevNode, DevOCP) are intentionally unmanaged sandboxes.

## Documentation

| Document | Contents |
|----------|----------|
| [Architecture](architecture.md) | System diagrams, environment map, GitOps flow |
| [Environments](environments.md) | Detailed breakdown of each environment |
| [Decisions](decisions.md) | Key decisions and rationale |
| [Repository Structure](repo-structure.md) | Infrastructure-as-code repo layout |
| [Roadmap](roadmap.md) | Open questions and future work |

## Tooling Stack

- **Infrastructure provisioning:** Terraform
- **System configuration:** Ansible
- **Container orchestration:** Komodo (Core + Periphery agents)
- **Version control:** Git (GitHub)

## Key Principles

1. **GitOps for managed environments** — Desired state lives in Git; Komodo reconciles actual state to match
2. **Promotion via file movement** — Move/copy TOML files between `dev/` → `test/` → `prod/` directories
3. **Isolation for sandboxes** — DevNode and DevOCP have no GitOps integration; they're for experimentation
4. **Dedicated management plane** — Komodo Controller runs on separate hardware (NUC) with no workloads
