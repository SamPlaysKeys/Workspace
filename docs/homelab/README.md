---
type: README-Note
---

# Homelab Infrastructure

Design, architecture, and operations for a greenfield homelab rebuild. Documentation is grouped **by topic**; use this page as the index.

## At a glance

The homelab is organized into **six logical environments** (VLANs) with different purposes:

| Environment | Purpose | Management |
|-------------|---------|------------|
| **Prod** | Production workloads | GitOps (Komodo) |
| **Test** | Pre-production validation | GitOps (Komodo) |
| **Dev** | Development, experimentation, OCP cluster | Mixed (Komodo for Docker, unmanaged for VMs/OCP) |
| **User** | Personal workstation | N/A |
| **IoT** | Smart home, cameras | Isolated |
| **DMZ** | Public-facing services (qBittorrent) | Isolated, restricted |

Prod, Test, and Dev (Docker containers) form a **promotion path** managed by Komodo. Dev also contains unmanaged nodes (VMs, OCP cluster) for experimentation.

---

## Overview

Orienting views of the whole lab.

| Document | Contents |
|----------|----------|
| [Homelab map](overview/homelab-map.md) | Hardware, VLANs, services, network topology |
| [Visual overview (HTML)](overview/homelab-graphic.html) | Interactive layout (open locally or publish as static HTML) |
| [Published graphic](https://samplayskeys.com/docs/homelab.html) | Same idea hosted on the public site (path may differ from this repo) |

---

## Network

Connectivity, edge, and remote access.

| Document | Contents |
|----------|----------|
| [Tailscale](network/tailscale.md) | Overlay, remote access, Docktail, shared access model |
| [UniFi configurations](network/unifi-configurations.md) | SSIDs, VLANs, controller notes |

---

## Platform

Architecture of environments, hardware, and infrastructure-as-code layout.

| Document | Contents |
|----------|----------|
| [Architecture](platform/architecture.md) | System diagrams, environment map, GitOps flow |
| [Environments](platform/environments.md) | Detailed breakdown of each environment |
| [Hardware](platform/hardware.md) | Physical inventory and specs |
| [Planned repository structure](platform/planned-repo-structure.md) | Infrastructure repo layout |

---

## Observability

Public status and operator dashboards.

| Document | Contents |
|----------|----------|
| [Observability index](observability/README.md) | Topic entry point |
| [Status site & operator dashboard](observability/status-and-operator-dashboard.md) | `status.samplayskeys.com`, VPS hosting, single pane of glass |

---

## Planning

Decisions, roadmap, ADRs, and working notes.

| Document | Contents |
|----------|----------|
| [Decisions](planning/decisions.md) | Key decisions and rationale |
| [ADRs](planning/adrs/README.md) | Architecture decision records (detailed, reference-friendly) |
| [Roadmap](planning/roadmap.md) | Open questions and future work |
| [Scratchpad](planning/scratchpad.md) | Informal notes and state |

---

## AI VM

Dedicated Debian VM with Quadro P5000 GPU passthrough, running LlamaFarm as the AI model management layer. Disposable — Ansible provisions, persistent storage holds models and data.

| Document | Contents |
|----------|----------|
| [Architecture](ai-vm/architecture.md) | VM specs, storage layout, GPU profile, provisioning flow |
| [ADR 0001](ai-vm/adr/0001-llamafarm-over-ansible.md) | LlamaFarm as AI model management layer |
| [Ansible playbook draft](ai-vm/ansible/playbook.yml) | Provisioning roles (os-hardening, nvidia-cuda, persistent-storage, llamafarm-install, llamafarm-service) |

---

## Smart home (IoT)

| Document | Contents |
|----------|----------|
| [IoT documentation](iot/README.md) | Home Assistant, Zigbee, automations, device decisions |

---

## Tooling stack

- **Infrastructure provisioning:** Terraform  
- **System configuration:** Ansible  
- **Container orchestration:** Komodo (Core + Periphery agents)  
- **Version control:** Git (GitHub)

## Key principles

1. **GitOps for managed environments** — Desired state lives in Git; Komodo reconciles actual state to match  
2. **Promotion via file movement** — Move/copy TOML files between `dev/` → `test/` → `prod/` directories  
3. **Isolation for sandboxes** — DevNode and DevOCP have no GitOps integration; they're for experimentation  
4. **Dedicated management plane** — Komodo Controller runs on separate hardware (NUC) with no workloads  
