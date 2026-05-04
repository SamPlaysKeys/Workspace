# Architecture

System diagrams and visual documentation for the homelab infrastructure.

---

## Environment Map

High-level view of all environments and their relationships.

```mermaid
flowchart TB
    subgraph ctrl[Komodo Controller - Dedicated NUC]
        KC[Komodo Core]
        KM[(MongoDB)]
    end

    subgraph managed[GitOps Managed Environments]
        subgraph prod[Prod]
            subgraph prod_unraid[Unraid Server]
                P_Storage[(Media Storage<br/>Movies, Shows, Anime)]
            end
            subgraph prod_mini[Prod MiniPC]
                P_Agent[Periphery Agent]
                P_Plex[Plex]
                P_Arr[*arr Suite]
                P_Home[Homepage]
                P_Other[Other Prod Apps...]
            end
        end

        subgraph test[Test]
            subgraph test_synology[Synology NAS]
                T_Storage[(Test Storage)]
            end
            subgraph test_mini[Test MiniPC]
                T_Agent[Periphery Agent]
                T_Plex[Plex - Test]
                T_Apps[Test Apps...]
            end
        end

        subgraph dev[Dev]
            subgraph dev_docker[DevDocker VM - on ProxMox]
                D_Agent[Periphery Agent]
                D_Arcane[Arcane UI]
                D_Containers[Experimental Containers]
            end
        end
    end

    subgraph unmanaged[Unmanaged / Sandbox Environments]
        subgraph proxmox_vms[ProxMox Server - VMs]
            subgraph dev_node[DevNode VMs]
                DN_VM1[Test VM 1]
                DN_VM2[Test VM 2]
                DN_VMn[...]
            end
        end

        subgraph dev_ocp[DevOCP - Isolated Cluster]
            subgraph ocp_cluster[3x MiniPC Cluster]
                OCP1[Control Plane]
                OCP2[Worker 1]
                OCP3[Worker 2]
            end
        end
    end

    KC -->|Manages| P_Agent
    KC -->|Manages| T_Agent
    KC -->|Manages| D_Agent
    P_Agent --> P_Plex
    P_Agent --> P_Arr
    P_Agent --> P_Home
    P_Agent --> P_Other
    T_Agent --> T_Plex
    T_Agent --> T_Apps
    D_Agent --> D_Arcane
    D_Agent --> D_Containers
    P_Storage -.->|NFS/SMB| prod_mini
    T_Storage -.->|NFS/SMB| test_mini

    style ctrl fill:#7c3aed,color:#fff
    style KC fill:#7c3aed,color:#fff
    style managed fill:#16a34a10,stroke:#16a34a,stroke-width:2px
    style prod fill:#22c55e15,stroke:#22c55e
    style test fill:#eab30815,stroke:#eab308
    style dev fill:#3b82f615,stroke:#3b82f6
    style unmanaged fill:#78716c10,stroke:#78716c,stroke-width:2px
    style proxmox_vms fill:#64748b15,stroke:#64748b
    style dev_ocp fill:#ef444415,stroke:#ef4444
    style P_Agent fill:#4a9eff,color:#fff
    style T_Agent fill:#4a9eff,color:#fff
    style D_Agent fill:#4a9eff,color:#fff
```

### Legend

| Color | Meaning |
|-------|---------|
| Purple | Komodo management plane (Controller) |
| Blue (fill) | Periphery agents |
| Green outer border | GitOps managed environments |
| Green inner | Prod environment |
| Yellow inner | Test environment |
| Blue inner | Dev environment (DevDocker) |
| Gray outer border | Unmanaged / sandbox environments |
| Gray inner | DevNode VMs (ProxMox) |
| Red inner | DevOCP (isolated, work-related) |

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

How changes flow from Git to running containers.

```mermaid
flowchart TD
    subgraph dev[Developer]
        A[Edit TOML/Compose] --> B[Git Push]
    end

    subgraph gh[GitHub Repo]
        B --> C[Repo Updated]
    end

    subgraph komodo[Komodo Controller]
        D[Komodo Core] -->|Poll Interval| E[Fetch Latest from Git]
        E --> F[Diff Against Current State]
        F --> G{Changes Detected?}
        G -->|No| D
        G -->|Yes| H{deploy: true?}
        H -->|Yes| I[Auto Execute Sync]
        H -->|No| J[Alert: Pending Changes]
        J -.->|Manual Approval| I
    end

    C -.->|Polled by| E

    subgraph target[Target Node]
        I --> K[Periphery Agent]
        K --> L[Pull Image]
        L --> M[Start/Update Container]
        M --> N[Health Check]
        N -->|Healthy| O[Running]
        N -->|Unhealthy| P[Alert / Rollback]
    end

    subgraph maint[Continuous Reconciliation]
        O --> Q[Komodo Monitors State]
        Q --> R{Drift from Git?}
        R -->|No| Q
        R -->|Yes| S[Reconcile to Declared State]
        S --> K
    end

    style D fill:#7c3aed,color:#fff
    style K fill:#4a9eff,color:#fff
    style O fill:#22c55e,color:#fff
    style P fill:#ef4444,color:#fff
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
