---
type: Reference
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

## Environment Comparison

| Aspect | Prod | Test | Dev | User | IoT |
|--------|------|------|-----|------|-----|
| **VLAN** | 10.0.1.0/24 | 10.0.2.0/24 | 10.0.3.0/24 | 10.0.10.0/24 | 10.0.4.0/24 |
| **Komodo managed** | Yes | Yes | Partial | No | No |
| **Promotion path** | — | → Prod | → Test | — | — |
| **Primary runtime** | Docker | Docker | Docker, VMs, OCP | Desktop | Embedded |
