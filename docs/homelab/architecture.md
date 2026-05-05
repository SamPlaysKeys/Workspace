# Architecture

System diagrams and visual documentation for the homelab infrastructure.

---

## Environment Map

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                        KOMODO CONTROLLER (NUC)                              │
│                     ┌──────────────┬───────────┐                            │
│                     │ Komodo Core  │  MongoDB  │                            │
│                     └──────┬───────┴───────────┘                            │
│                            │ manages                                        │
└────────────────────────────┼────────────────────────────────────────────────┘
                             │
        ┌────────────────────┼────────────────────┐
        │                    │                    │
        ▼                    ▼                    ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│                      GITOPS MANAGED ENVIRONMENTS                            │
│                                                                             │
│  ┌─────────────────────┐ ┌─────────────────────┐ ┌───────────────────────┐  │
│  │        PROD         │ │        TEST         │ │          DEV          │  │
│  │                     │ │                     │ │                       │  │
│  │  ┌───────────────┐  │ │  ┌───────────────┐  │ │  ┌─────────────────┐  │  │
│  │  │  Prod MiniPC  │  │ │  │  Test MiniPC  │  │ │  │ DevDocker VM    │  │  │
│  │  │ [Periphery]   │  │ │  │ [Periphery]   │  │ │  │ (on ProxMox)    │  │  │
│  │  │               │  │ │  │               │  │ │  │ [Periphery]     │  │  │
│  │  │ • Plex        │  │ │  │ • Plex (test) │  │ │  │                 │  │  │
│  │  │ • *arr Suite  │  │ │  │ • Test Apps   │  │ │  │ • Arcane UI     │  │  │
│  │  │ • Homepage    │  │ │  │               │  │ │  │ • Experimental  │  │  │
│  │  │ • ...         │  │ │  │               │  │ │  │                 │  │  │
│  │  └───────────────┘  │ │  └───────────────┘  │ │  └─────────────────┘  │  │
│  │         ▲           │ │         ▲           │ │                       │  │
│  │         │ NFS/SMB   │ │         │ NFS/SMB   │ │                       │  │
│  │  ┌──────┴────────┐  │ │  ┌──────┴────────┐  │ │                       │  │
│  │  │ Unraid Server │  │ │  │ Synology NAS  │  │ │                       │  │
│  │  │ (Media)       │  │ │  │ (Test Data)   │  │ │                       │  │
│  │  └───────────────┘  │ │  └───────────────┘  │ │                       │  │
│  └─────────────────────┘ └─────────────────────┘ └───────────────────────┘  │
│                                                                             │
│  Promotion path: DEV ──────────────▶ TEST ──────────────▶ PROD              │
└─────────────────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────────────────┐
│                     UNMANAGED / SANDBOX ENVIRONMENTS                        │
│                                                                             │
│  ┌────────────────────────────────────┐  ┌───────────────────────────────┐  │
│  │     DEVNODE (ProxMox VMs)          │  │    DEVOCP (Isolated)          │  │
│  │                                    │  │                               │  │
│  │  • Test VM 1                       │  │  3x MiniPC Cluster:           │  │
│  │  • Test VM 2                       │  │  • Control Plane              │  │
│  │  • ...                             │  │  • Worker 1                   │  │
│  │                                    │  │  • Worker 2                   │  │
│  │  (bugfixes, themes, ad-hoc)        │  │                               │  │
│  │                                    │  │  (OpenShift - work projects)  │  │
│  └────────────────────────────────────┘  └───────────────────────────────┘  │
│                                                                             │
│  No Komodo. No GitOps. Just sandboxes.                                      │
└─────────────────────────────────────────────────────────────────────────────┘
```

### Key Points

- **Komodo Controller** sits outside the environment groups — it manages but is not part of workload infrastructure
- **Managed environments** (Prod, Test, Dev) have Periphery agents and are GitOps-controlled
- **Dev (DevDocker)** runs on ProxMox but is still Komodo-managed — promotion path to Test → Prod
- **Unmanaged environments** are sandboxes — no Periphery agents, no GitOps
- **DevNode** VMs on ProxMox are for ad-hoc testing (bugfixes, themes) — no container management
- **DevOCP** is fully isolated — separate hardware (3x MiniPCs), no Komodo integration
- **Storage** is accessed via network mounts from Unraid (Prod) and Synology (Test)

---

## GitOps Flow

```
┌──────────────────┐
│    Developer     │
│                  │
│  Edit TOML or    │
│  Compose file    │
│        │         │
│        ▼         │
│    Git Push      │
└────────┬─────────┘
         │
         ▼
┌──────────────────┐
│   GitHub Repo    │
│                  │
│  Repo Updated    │
└────────┬─────────┘
         │
         │ (polled by)
         ▼
┌──────────────────────────────────────────────────────┐
│                  KOMODO CONTROLLER                   │
│                                                      │
│  ┌────────────┐    ┌─────────────────────────────┐   │
│  │   Komodo   │───▶│  Fetch latest from Git      │   │
│  │    Core    │    └──────────────┬──────────────┘   │
│  │            │                   │                  │
│  │  (polling) │    ┌──────────────▼──────────────┐   │
│  │            │    │  Diff against current state │   │
│  └────────────┘    └──────────────┬──────────────┘   │
│                                   │                  │
│                    ┌──────────────▼──────────────┐   │
│                    │      Changes detected?      │   │
│                    └──────────────┬──────────────┘   │
│                          No ┌─────┴─────┐ Yes        │
│                             │           │            │
│                          (loop)  ┌──────▼────────┐   │
│                                  │ deploy: true? │   │
│                                  └───────┬───────┘   │
│                                 No ┌─────┴─────┐ Yes │
│                                    │           │     │
│                    ┌───────────────▼───┐ ┌─────▼───┐ │
│                    │ Alert: Pending    │ │  Auto   │ │
│                    │ (manual approval) │ │ Deploy  │ │
│                    └─────────┬─────────┘ └────┬────┘ │
│                              │                │      │
│                              └───────┬────────┘      │
└──────────────────────────────────────┼───────────────┘
                                       │
                                       ▼
┌──────────────────────────────────────────────────────┐
│                    TARGET NODE                       │
│                                                      │
│  ┌─────────────┐   ┌─────────────┐   ┌───────────┐   │
│  │  Periphery  │──▶│ Pull Image  │──▶│  Start/   │   │
│  │    Agent    │   │             │   │  Update   │   │
│  └─────────────┘   └─────────────┘   └─────┬─────┘   │
│                                            │         │
│                              ┌─────────────▼───────┐ │
│                              │    Health Check     │ │
│                              └──────────┬──────────┘ │
│                           Healthy ┌─────┴─────┐ Fail │
│                                   │           │      │
│                           ┌───────▼───┐ ┌─────▼────┐ │
│                           │  Running  │ │  Alert/  │ │
│                           │           │ │ Rollback │ │
│                           └─────┬─────┘ └──────────┘ │
└─────────────────────────────────┼────────────────────┘
                                  │
                                  ▼
┌──────────────────────────────────────────────────────┐
│               CONTINUOUS RECONCILIATION              │
│                                                      │
│      Komodo monitors ──▶ Drift detected? ──▶ Fix     │
│            ▲                                  │      │
│            └──────────────────────────────────┘      │
└──────────────────────────────────────────────────────┘
```

### Flow Explanation

1. **Developer** edits TOML or Compose files and pushes to GitHub
2. **Komodo Core** polls GitHub on a configured interval
3. **Diff detection** compares Git state to current deployed state
4. **Deployment decision:**
   - If `deploy: true` in the resource config → auto-deploy
   - If `deploy: false` → alert about pending changes, wait for manual approval
5. **Periphery Agent** on the target node pulls the image and starts/updates the container
6. **Health check** determines success or triggers alerts/rollback
7. **Continuous reconciliation** monitors for drift and corrects it automatically

---

## Hardware Summary

| Component | Role | Environment |
|-----------|------|-------------|
| **NUC** | Komodo Controller | Management plane |
| **MiniPC (Prod)** | Container host | Prod |
| **Unraid Server** | Media storage | Prod |
| **MiniPC (Test)** | Container host | Test |
| **Synology NAS** | Test storage | Test |
| **ProxMox Server** | VM host | Dev, DevNode |
| **3× MiniPC Cluster** | OpenShift | DevOCP |

---

## Network Topology

*To be documented — network segmentation, VLANs, access controls between environments.*
