---
type: Reference
---
# Migration Timeout Calculation Reference

## The Formula

KubeVirt/OpenShift Virtualization uses `CompletionTimeoutPerGiB` to calculate the maximum allowed duration for a live migration:

```
Total Timeout (seconds) = CompletionTimeoutPerGiB × VM Memory (GiB)
```

## Example Calculations

| VM Size | Value | Total Timeout |
|---------|-------|---------------|
| 16 GiB | 5 | 80 seconds |
| 64 GiB | 5 | 320 seconds |
| 128 GiB | 5 | 640 seconds |
| 256 GiB | 5 | 1,280 seconds (~21 min) |

---

## Theoretical Transfer Time

At a given network bandwidth, the theoretical time to transfer VM memory:

```
Transfer Time (sec) = VM Memory (GiB) × 1024 / Bandwidth (MiB/s)
```

| Network | Bandwidth | Transfer 1 GiB |
|---------|-----------|----------------|
| 1 Gbps | 125 MiB/s | ~8.2 seconds |
| 10 Gbps | 1,250 MiB/s | ~0.8 seconds |
| 25 Gbps | 3,125 MiB/s | ~0.3 seconds |

---

## Why Timeout > Transfer Time

Migration involves more than a single memory transfer:

1. **Iterative pre-copy** — Memory pages copied repeatedly while VM runs; dirty pages must be re-sent
2. **Convergence wait** — Migration waits for dirty-page rate to drop below threshold
3. **Final cutover** — Brief pause to sync remaining state before switching to target node
4. **Protocol overhead** — Framing, potential compression, TLS encryption

### Dirty-Page Iteration Multiplier

| Workload Profile | Typical Iterations | Effective Multiplier |
|------------------|-------------------|---------------------|
| Idle/low churn | 1-2 | 1-2x base transfer |
| Moderate churn | 2-4 | 2-4x base transfer |
| High churn | 5+ | 5-10x base transfer |
| Failing to converge | Infinite | Never finishes |

### Realistic Timeout Calculation

```
Timeout = Base Transfer × Iteration Multiplier + Convergence Buffer
```

**Example: 64 GiB VM at 10 Gbps, moderate churn**

| Component | Calculation | Value |
|-----------|-------------|-------|
| Base transfer | 64 GiB × 0.8 sec/GiB | 51 sec |
| Iteration multiplier | 51 × 3 | 153 sec |
| Convergence buffer | Fixed margin | 30 sec |
| **Total** | | **183 sec** |
| **Per GiB** | 183 / 64 | **~2.9 sec/GiB** |

---

## Starting Values by Environment

| Network Configuration | Recommended Value | Rationale |
|----------------------|-------------------|-----------|
| Dedicated 10+ Gbps | 2-5 | Fast transfer; adjust based on observed churn |
| Dedicated 1-10 Gbps | 5-10 | Bandwidth variance; longer convergence |
| Shared/overcommitted | 20-50 | Contention with other traffic |
| Unknown/baseline | 50-100 | Conservative; measure and tune down |

---

## Calibration Method

### Step 1: Measure Baseline

Run dry-run or test migrations during a maintenance window:

```bash
# Dry-run migration (estimates duration)
virtctl migrate <vm-name> --dry-run

# Monitor actual migration
oc get vmim <migration-name> -o yaml | grep -A 10 status
```

### Step 2: Calculate Observed Sec/GiB

```
Observed Sec/GiB = Migration Duration (sec) / VM Memory (GiB)
```

### Step 3: Set Value with Safety Margin

```
CompletionTimeoutPerGiB = Observed Sec/GiB × 1.5 (50% safety margin)
```

### Step 4: Monitor and Adjust

- If migrations timeout → increase value
- If migrations complete in < 50% of timeout → decrease value
- Track per workload class for consistent tuning

---

## HCO Configuration

Set cluster-wide default via HyperConverged CR.

**See:** `artifacts/openshift/openshift-virtualization-upgrade/manifests/migration-policies/hco-cluster-default.yml` for a complete example with all configurable fields.

Override per workload class via `MigrationPolicy` CR — see `artifacts/openshift/openshift-virtualization-upgrade/manifests/migration-policies/` for baseline, enhanced, auto-converge, and post-copy policy examples.

---

## References

### KubeVirt Documentation
- [Live Migration Configuration](https://kubevirt.io/user-guide/operations/live_migration/#configuration) — Migration policy settings including `completionTimeoutPerGiB`, `parallelMigrationsPerCluster`, `autoConverge`, `allowPostCopy`
- [MigrationPolicy CRD](https://kubevirt.io/user-guide/operations/live_migration/#migrationpolicies) — Per-workload migration policy definitions

### OpenShift Virtualization Documentation
- [Configuring Live Migration](https://docs.openshift.com/container-platform/latest/virt/virtual_machines/virt-node-maintenance-virt.html#virt-configuring-live-migration-policies_virt-node-maintenance-virt) — OpenShift-specific migration policy configuration
- [HyperConverged CR Configuration](https://docs.openshift.com/container-platform/latest/virt/install/virt-installing-virt-operator.html#virt-hyperconverged-cr_virt-installing-virt-operator) — Cluster-wide live migration settings via HCO

### QEMU Documentation
- [Pre-copy vs Post-copy Migration](https://wiki.qemu.org/Features/PostCopyLiveMigration) — Technical background on migration modes and trade-offs
