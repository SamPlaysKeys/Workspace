---
type: Reference
layout: page
title: Environments
category: Homelab
status: Active
---


# Environments

Detailed breakdown of each environment (VLAN) in the homelab.

---

## Managed Environments (GitOps)

These environments have Komodo Periphery agents on Docker hosts, managed via GitOps. Changes flow through Git → Komodo Core → Periphery Agent → Container.

### Prod

**Purpose:** Production workloads — services that need to be reliable and always available.

**VLAN:** 10.0.1.0/24

**Hardware:**
- **Compute:** MiniPC (LenovoMini 1) running Docker containers
- **Storage:** UnRaid NAS (media storage: movies, shows, anime)
- **Management:** Intel NUC (Komodo Controller)

**Workloads:**
- Plex Media Server
- *arr suite (Sonarr, Radarr, Prowlarr, etc.)
- Other production services (see [homelab-map.md](../overview/homelab-map.md) for full list)

**Network access:** UnRaid NAS provides NFS/SMB shares to the MiniPC for media access.

---

### Test

**Purpose:** Pre-production validation — test changes before promoting to Prod.

**VLAN:** 10.0.2.0/24

**Hardware:**
- **Compute:** MiniPC (LenovoMini 2) running Docker containers
- **Storage:** Synology NAS

**Workloads:** Mirrors Prod where needed for testing. Not all Prod services need a Test instance.

**Promotion:** Validated apps/configs move from Test → Prod by copying/moving TOML files.

---

### Dev

**Purpose:** Development, experimentation, and work-related projects.

**VLAN:** 10.0.3.0/24

**Hardware:**
- **ProxMox Host** — Runs VMs for Docker and sandbox work
  - DevDocker VM — Komodo-managed Docker containers (promotion path to Test → Prod)
  - DevNode VMs — Unmanaged sandbox for testing bugfixes, themes, OS-level changes
- **MiniPC cluster (LenovoMini 3–5)** — OpenShift cluster for work-related projects

**Managed vs Unmanaged:**

| Node | Komodo Managed | Purpose |
|------|----------------|---------|
| DevDocker VM | Yes | Container dev with promotion path |
| DevNode VMs | No | Throwaway VM sandbox |
| OCP Cluster | No | OpenShift for work projects |

**Note:** The OCP cluster uses OpenShift's built-in GitOps (pulls from Git) but is not managed by Komodo.

---

## Other Environments

These are network segments without Komodo-managed workloads.

### User

**Purpose:** Personal workstation.

**VLAN:** 10.0.10.0/24

**Hardware:** Gaming PC

**Isolation:** Isolated from Prod, Test, Dev, and IoT.

---

### IoT

**Purpose:** Smart home devices and cameras.

**VLAN:** 10.0.4.0/24

**Hardware:** Reolink cameras, Google Home devices

**Isolation:** Fully isolated from all other VLANs. Cannot reach Prod, Test, or Dev.

---

### DMZ

**Purpose:** Public-facing services that need internet exposure — qBittorrent for Internet Archive torrent backups, potentially other public endpoints.

**VLAN:** 10.0.5.0/24

**Hardware:** Two deployment options:

**Option A — Shared NICs via VLAN tagging:**
- DMZ containers/VMs run on existing hosts (LenovoMini 1, ProxMox) with the DMZ VLAN tagged on a shared physical NIC
- qBittorrent runs as a Docker container on LenovoMini 1 with a DMZ-network interface
- Lower cost, no additional hardware, but shares failure domain with Prod/Dev

**Option B — Dedicated seed box:**
- A dedicated MiniPC on the DMZ VLAN, physically isolated from all other environments
- Runs qBittorrent (and future DMZ services) on a baremetal OS or minimal Docker setup
- Higher isolation, no shared attack surface with Prod/Dev, but additional hardware cost

**Shared Storage:**
- DMZ services write downloaded content to an NFS export on the UnRaid NAS (Prod VLAN)
- A specific firewall exception allows DMZ → UnRaid NAS on NFS port 2049
- Prod services (Plex, *arr) access the same export for processing — no data transfer through DMZ needed beyond the initial write

**Firewall:**
- DMZ → External: Allowed (torrent traffic, updates)
- External → DMZ: Restricted to specific ports (torrent DHT/TCP)
- DMZ → Internal: **Blocked by default**. Controlled exception for DMZ → UnRaid NAS (NFS)

---

## Environment Comparison

| Aspect | Prod | Test | Dev | User | IoT | DMZ |
|--------|------|------|-----|------|-----|-----|
| **VLAN** | 10.0.1.0/24 | 10.0.2.0/24 | 10.0.3.0/24 | 10.0.10.0/24 | 10.0.4.0/24 | 10.0.5.0/24 |
| **Komodo managed** | Yes | Yes | Partial | No | No | No |
| **Promotion path** | — | → Prod | → Test | — | — | — |
| **Primary runtime** | Docker | Docker | Docker, VMs, OCP | Desktop | Embedded | Docker / baremetal |
