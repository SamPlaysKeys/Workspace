---
type: Roadmap
status: Active
Date: 2026-07-31
category: Architecture
layout: page
title: GitOps Cluster Consolidation & Version Control Roadmap
---

# GitOps Cluster Consolidation & Version Control Roadmap

This roadmap outlines the strategic phased approach for consolidating disparate, tag-drifted, or unmanaged GitOps clusters (environments) back into a fully version-controlled lifecycle managed off of a single `main` branch with tagging.

## Objective
To eliminate repository drift, simplify environment configuration, and establish a clear promotion path by:
1. Baselining all clusters onto a single trunk (`main`) with no active tags or branches.
2. Cleaning up all unused/stale git branches and tags.
3. Separating testing environments onto a dedicated long-lived `Testing` branch.
4. Pinning all remaining environments (production, shared services) to a new, immutable release tag (`v2.0`).

---

## High-Level Flowchart

```mermaid
flowchart TD
    %% Current State
    subgraph Current [Current State: Drift & Disparate Mapping]
        C_Prod[Prod Clusters / Tags/Branches]
        C_Test[Test Clusters / Diverse Tags]
        C_Unmanaged[Unmanaged Clusters / No Sync]
    end

    %% Phase 1
    subgraph Phase1 [Phase 1: Trunk Consolidation]
        Main_Branch[(Git Repo: main branch)]
        P1_Sync[Point ALL Cluster GitOps/ResourceSync to main branch]
        P1_Clean[Prune all stale tags & inactive branches]
    end

    %% Phase 2
    subgraph Phase2 [Phase 2: Testing Separation]
        Test_Branch[(Git Repo: Testing branch)]
        P2_Sync[Point Test Clusters to 'Testing' branch]
    end

    %% Phase 3
    subgraph Phase3 [Phase 3: Version Stabilization]
        V2_Tag[(Git Tag: v2.0)]
        P3_Sync[Point Production & Other Clusters to 'v2.0' tag]
    end

    %% Transitions
    Current -->|Align and Point to main| P1_Sync
    P1_Sync -->|Prune old Git refs| P1_Clean
    P1_Clean -->|Create 'Testing' from main| Test_Branch
    Test_Branch -->|Configure Sync| P2_Sync
    P1_Clean -->|Tag main as v2.0| V2_Tag
    V2_Tag -->|Configure Sync| P3_Sync

    %% Styling
    style Current fill:#fdd,stroke:#f66,stroke-width:2px;
    style Phase1 fill:#ffd,stroke:#dd6,stroke-width:2px;
    style Phase2 fill:#dfd,stroke:#6b6,stroke-width:2px;
    style Phase3 fill:#ddf,stroke:#66b,stroke-width:2px;
```

---

## Phased Consolidation Plan

### Phase 1: Trunk Consolidation (Stabilization Baseline)
**Goal:** Consolidate all GitOps controllers/ResourceSync configurations to point strictly to the `main` branch with zero tag/revision overrides.

1. **Map and Align Configurations:**
   - Update all cluster GitOps configs (e.g., Komodo `[[resource_sync]]` target revisions or ArgoCD target revisions) to use the `main` branch.
   - Ensure all cluster-specific directories (e.g., `komodo/prod/`, `komodo/test/`, `komodo/dev/`) are unified on `main`.
2. **Reconcile and Validate:**
   - Allow GitOps controllers to reconcile. Confirm that all clusters are running stable configurations served purely from `main`.
3. **Repository Pruning:**
   - Delete all unused, stale, and tracking branches.
   - Purge old tags from the remote git origin to prevent metadata clutter and reference confusion.

### Phase 2: Testing Isolation
**Goal:** Branch out the testing clusters to a dedicated, long-lived branch to isolate validation work from main production configurations.

1. **Branch Creation:**
   - Branch `Testing` directly from the cleaned and verified `main` branch.
2. **Target Re-pointing:**
   - Modify the GitOps sync configurations of your **Test Clusters** to track the `Testing` branch instead of `main`.
3. **Promotion Workflow Baseline:**
   - Features/workloads are committed to `main` (or individual feature branches), validated, merged to `main`, and then merged from `main` to `Testing` for soak tests.

### Phase 3: Release Pinning (`v2.0` Tag)
**Goal:** Establish release stability by pinning production and all non-test clusters to an immutable point-in-time release.

1. **Tag Generation:**
   - Tag the stable `main` branch with the new version tag: `v2.0`.
2. **Target Re-pointing:**
   - Update the GitOps sync configurations for all remaining clusters (e.g., Production, Shared Core Services) to target `refs/tags/v2.0`.
3. **Establish Lifecycle Operations:**
   - All subsequent production updates are promoted from `Testing` back to `main`, and then promoted to production by updating/moving the target tag (e.g., `v2.1`, `v2.2`) or updating the tag pointer.

---

## Key Benefits
* **Clean Git Slate:** Drastically reduces active git refs, making branch history easy to read and manage.
* **Predictable Promotion:** Features move from `main` $\rightarrow$ `Testing` branch $\rightarrow$ `v2.0` tag.
* **Auditability:** Clear division of what is being tested versus what is running in production based purely on git references.


---

## Long-Term Management

To ensure continuous operational stability and transparency, clusters are categorized by environment type and mapped to specific Git target references, utilizing a flat repository structure for environment settings.

### Cluster Target References

Different environments track different Git references depending on their operational requirements and strictness levels:

1. **Test Clusters:**
   - **Target Reference:** `Testing` branch.
   - **Usage:** Serves as the immediate validation target for developers and GitOps sync, ensuring new features and configurations are continuously integrated and tested.

2. **Pre-Production and Normal Production Clusters:**
   - **Target Reference:** `Main` branch.
   - **Usage:** Serves as the primary stable branch. Pre-production acts as the final validation gate, while normal production tracks the main trunk directly for rapid, reliable delivery of verified features.

3. **Strictly Managed Clusters:**
   - **Target Reference:** Tagged releases (e.g., `v2.x`).
   - **Usage:** Pin to immutable release tags to guarantee absolute control over rollout schedules and facilitate rigorous change management.

### Flat Repository Structure

Instead of deep hierarchies or branching drift for configuration variants, the repository is organized with a **flat directory structure** at its root or specific config directory (e.g., `komodo/` or `gitops/`). 

For each environment, a dedicated, self-contained subdirectory contains all specific configurations and override settings:

```text
gitops-repo/
├── test/                # Settings specific to Test clusters (tracks 'Testing' branch)
├── preprod/             # Settings specific to Pre-production (tracks 'Main' branch)
├── prod/                # Settings specific to Normal Production (tracks 'Main' branch)
├── prod-strict/         # Settings specific to Strictly Managed Production (tracks immutable tags)
└── shared/              # Shared base resources or templates referenced by environments
```

This flat design guarantees that:
* **High Visibility:** All environment-specific settings are completely visible in their respective subdirectories.
* **No Hidden Overrides:** Eliminates nested inheritance or complex drift, making configurations easy to audit, compare, and modify.
* **Declarative Consistency:** All target references can read from the same commit or release tag while cleanly resolving environment-specific configurations via their designated subdirectory.
