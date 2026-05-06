# Homelab Infrastructure

This documentation covers the design, architecture, and operational strategy for a greenfield homelab rebuild.

## Overview

The homelab is organized into **five logical environments** (VLANs) with different purposes:

| Environment | Purpose | Management |
|-------------|---------|------------|
| **Prod** | Production workloads | GitOps (Komodo) |
| **Test** | Pre-production validation | GitOps (Komodo) |
| **Dev** | Development, experimentation, OCP cluster | Mixed (Komodo for Docker, unmanaged for VMs/OCP) |
| **User** | Personal workstation | N/A |
| **IoT** | Smart home, cameras | Isolated |

Prod, Test, and Dev (Docker containers) form a **promotion path** managed by Komodo. Dev also contains unmanaged nodes (VMs, OCP cluster) for experimentation.

## Documentation

| Document | Contents |
|----------|----------|
| [Architecture](architecture.md) | System diagrams, environment map, GitOps flow |
| [Environments](environments.md) | Detailed breakdown of each environment |
| [Homelab Map](homelab-map.md) | Hardware, VLANs, services, and network topology |
| [Decisions](decisions.md) | Key decisions and rationale |
| [Repository Structure](repo-structure.md) | Infrastructure-as-code repo layout |
| [Tailscale](tailscale.md) | Network connectivity, remote access, shared access model |
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
