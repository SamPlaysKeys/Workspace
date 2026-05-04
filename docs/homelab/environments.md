# Environments

Detailed breakdown of each environment in the homelab.

---

## Managed Environments (GitOps)

These environments have Komodo Periphery agents installed and are managed via GitOps. Changes flow through Git → Komodo Core → Periphery Agent → Container.

### Prod

**Purpose:** Production workloads — services that need to be reliable and always available.

**Hardware:**
- **Compute:** MiniPC running Docker containers
- **Storage:** Unraid server (media storage: movies, shows, anime)

**Workloads:** (planned)
- Plex Media Server
- *arr suite (Sonarr, Radarr, Prowlarr, etc.)
- Homepage dashboard
- Other production services

**Network access:** Unraid provides NFS/SMB shares to the MiniPC for media access.

---

### Test

**Purpose:** Pre-production validation — test changes before promoting to Prod.

**Hardware:**
- **Compute:** MiniPC running Docker containers
- **Storage:** Synology NAS

**Workloads:** Mirrors Prod where needed for testing. Not all Prod services need a Test instance.

**Promotion:** Validated apps/configs move from Test → Prod by copying/moving TOML files.

---

### Dev (DevDocker)

**Purpose:** Container development and experimentation with a promotion path.

**Hardware:**
- **Compute:** VM on ProxMox server running Docker
- **UI:** Arcane (Docker management interface)

**Workloads:** Experimental containers, new apps being evaluated, development builds.

**Promotion:** Validated apps move from Dev → Test → Prod.

**Note:** This is the only dev environment with Komodo management. It's for container work that may eventually go to production.

---

## Unmanaged Environments (Sandboxes)

These environments have no Komodo integration. They're for experimentation that doesn't need GitOps or won't promote to production.

### DevNode

**Purpose:** VM sandbox for testing bugfixes, themes, and other non-container work.

**Hardware:**
- VMs on ProxMox server

**Use cases:**
- Testing OS-level changes
- Theme development
- Bugfix validation
- Anything that needs a throwaway VM

**Management:** Fully manual. Spin up VMs as needed, destroy when done.

---

### DevOCP

**Purpose:** OpenShift development for work-related projects.

**Hardware:**
- 3× MiniPC cluster running OpenShift

**Architecture:**
- 1 control plane node
- 2 worker nodes

**Isolation:** Fully isolated from the rest of the homelab. Separate network, no Komodo integration. This is for work projects, not personal homelab workloads.

**Management:** OpenShift's built-in GitOps (pulls from Git, but not managed by Komodo).

---

## Environment Comparison

| Aspect | Prod | Test | Dev | DevNode | DevOCP |
|--------|------|------|-----|---------|--------|
| **Komodo managed** | Yes | Yes | Yes | No | No |
| **Promotion path** | — | → Prod | → Test | None | None |
| **Hardware** | MiniPC + Unraid | MiniPC + Synology | ProxMox VM | ProxMox VMs | 3× MiniPC |
| **Runtime** | Docker | Docker | Docker | VMs | OpenShift |
| **Purpose** | Reliability | Validation | Experimentation | Sandbox | Work projects |
