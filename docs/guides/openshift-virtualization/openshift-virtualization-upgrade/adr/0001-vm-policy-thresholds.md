---
type: ADR
---
# ADR 0001: VM Policy Thresholds for Live Migration Classification

**Status:** Proposed  
**Date:** 2026-07-15

## Context

OpenShift Virtualization clusters require a repeatable method to classify VMs and assign appropriate live migration policies. Migration behavior varies significantly based on:

1. **Memory footprint** — Larger VMs require more time to transfer RAM state.
2. **Dirty-page rate** — High-churn workloads may fail to converge with standard pre-copy.
3. **Business criticality** — Some workloads tolerate auto-converge CPU throttling; others do not.

Without defined thresholds, operators must make ad-hoc decisions during maintenance windows, leading to inconsistent behavior, failed migrations, and extended downtime.

## Decision Drivers

1. **Near-zero-downtime objective** — Migrations must complete within acceptable maintenance windows.
2. **Predictable operations** — Classification should be automatable and reproducible.
3. **Risk awareness** — Post-copy and auto-converge have trade-offs that require explicit approval for certain workload classes.
4. **Empirical tuning** — Thresholds are starting points; they will be refined based on observed migration metrics.

## Decision

Define VM classification thresholds based on memory size and dirty-page rate, mapped to migration policy profiles.

### Memory Size Thresholds

| Class | Memory Range | Default Policy | Rationale |
|-------|--------------|----------------|-----------|
| **Small** | < 16 GB | Baseline pre-copy | Low memory volume; migration completes quickly in most cases |
| **Medium** | 16-64 GB | Enhanced pre-copy | Longer timeout, bandwidth tuning; no aggressive measures needed |
| **Large** | 64-256 GB | Auto-converge eligible | Migration may struggle to converge; CPU throttling acceptable |
| **Very Large** | > 256 GB | Post-copy or manual planning | Standard migration unlikely to complete in reasonable time; requires approval |

### Dirty-Page Rate Thresholds

| Dirty-Rate Profile | Threshold | Impact | Policy Adjustment |
|--------------------|-----------|--------|-------------------|
| **Low churn** | < 500 pages/sec | Migration converges easily | Standard policy |
| **Moderate churn** | 500-2000 pages/sec | May need longer timeout | Increase `completionTimeoutPerGiB` |
| **High churn** | 2000-5000 pages/sec | Pre-copy may not converge | Enable `autoConverge` |
| **Extreme churn** | > 5000 pages/sec | Pre-copy unlikely to succeed | Enable `postCopy` (with approval) or schedule downtime |

### Combined Classification Matrix

| Memory \ Dirty-Rate | Low | Moderate | High | Extreme |
|---------------------|-----|----------|------|---------|
| **Small (<16 GB)** | Baseline | Baseline + timeout | Auto-converge | Auto-converge |
| **Medium (16-64 GB)** | Baseline + timeout | Enhanced | Auto-converge | Auto-converge + extended timeout |
| **Large (64-256 GB)** | Enhanced | Auto-converge | Auto-converge | Post-copy (approved) |
| **Very Large (>256 GB)** | Auto-converge | Auto-converge | Post-copy (approved) | Post-copy or maintenance window |

## Policy Profile Definitions

> **See:** `../migration-timeout-calculation.md` for detailed calculation methodology and calibration guidance.

All profiles use environment-appropriate timeout values. The values below assume a **dedicated 10+ Gbps migration network**. Adjust `completionTimeoutPerGiB` based on your network configuration:

| Network Config | Recommended Starting Value |
|----------------|---------------------------|
| Dedicated 10+ Gbps | 2-5 |
| Dedicated 1-10 Gbps | 5-10 |
| Shared/overcommitted | 20-50 |
| Unknown baseline | 50-100 |

### Profile 1: Baseline Pre-Copy

Suitable for small VMs with low-to-moderate churn.

```yaml
apiVersion: kubevirt.io/v1alpha1
kind: MigrationPolicy
metadata:
  name: baseline-vm
spec:
  completionTimeoutPerGiB: 5  # Adjust for your network
  parallelMigrationsPerCluster: 5
  parallelOutboundMigrationsPerNode: 2
  autoConverge: false
  allowPostCopy: false
```

### Profile 2: Enhanced Pre-Copy

Suitable for medium VMs or workloads needing longer convergence windows.

```yaml
apiVersion: kubevirt.io/v1alpha1
kind: MigrationPolicy
metadata:
  name: enhanced-vm
spec:
  completionTimeoutPerGiB: 10  # Adjust for your network
  parallelMigrationsPerCluster: 3
  parallelOutboundMigrationsPerNode: 1
  bandwidthPerMigration: "256Mi"
  autoConverge: false
  allowPostCopy: false
```

### Profile 3: Auto-Converge Enabled

Suitable for large VMs or high-churn workloads that struggle to converge.

```yaml
apiVersion: kubevirt.io/v1alpha1
kind: MigrationPolicy
metadata:
  name: autoconverge-vm
spec:
  completionTimeoutPerGiB: 15  # Adjust for your network
  parallelMigrationsPerCluster: 2
  parallelOutboundMigrationsPerNode: 1
  bandwidthPerMigration: "512Mi"
  autoConverge: true
  allowPostCopy: false
```

### Profile 4: Post-Copy Enabled

Suitable for very large VMs or extreme-churn workloads. Requires approval.

```yaml
apiVersion: kubevirt.io/v1alpha1
kind: MigrationPolicy
metadata:
  name: postcopy-vm
spec:
  completionTimeoutPerGiB: 20  # Adjust for your network
  parallelMigrationsPerCluster: 1
  parallelOutboundMigrationsPerNode: 1
  bandwidthPerMigration: "0"  # Unlimited
  autoConverge: true
  allowPostCopy: true
```

## Timeout Calibration

Timeout values are environment-dependent. Calibrate using:

1. **Measure baseline:** Run dry-run migrations during test windows
2. **Calculate observed sec/GiB:** `Duration / VM Memory`
3. **Set with safety margin:** `Observed × 1.5`

**Example:**
- Observed migration: 180 seconds for 64 GiB VM
- Observed sec/GiB: 180 / 64 = 2.8
- Set value: 2.8 × 1.5 ≈ 4 (round up to 5)

See `../migration-timeout-calculation.md` for full methodology.

## Trade-Offs

### Auto-Converge

| Benefit | Risk |
|---------|------|
| Forces migration to converge by throttling guest CPU | Guest workload experiences performance degradation during migration |
| Avoids migration timeout failures | CPU-sensitive workloads (databases, real-time) may be impacted |

### Post-Copy

| Benefit | Risk |
|---------|------|
| Allows very large or high-churn VMs to migrate successfully | If failure occurs during post-copy phase, VM state may be unrecoverable |
| Reduces total migration time for large VMs | Guest experiences on-demand memory fetch latency (slower warm-up) |
| | Requires explicit approval for production workloads |

## Labeling Convention

VMs should be labeled for policy assignment:

```yaml
metadata:
  labels:
    migration-policy: autoconverge-vm
    vm-class: large
    dirty-rate-profile: high
```

Label selectors in `MigrationPolicy`:

```yaml
spec:
  selectors:
    virtualMachineInstanceSelector:
      migration-policy: autoconverge-vm
```

## Measurement Method

Dirty-page rate must be measured empirically before classification:

```bash
# During migration dry-run or observation window
virtctl migrate <vm-name> --dry-run

# Or via QEMU monitor
virsh qemu-monitor-command <vm-name> --hmp "info migrate"
```

Tools:
- `virtctl migrate --dry-run` — Estimates migration duration and dirty-rate
- QEMU Guest Agent — Reports memory statistics
- OpenShift Monitoring — Prometheus metrics for migration duration

## Consequences

### Positive

- Consistent, repeatable classification of VMs across clusters
- Clear guidance for operators during maintenance planning
- Explicit approval process for high-risk migration modes (post-copy)
- Empirical foundation for future threshold tuning

### Negative

- Thresholds are estimates; real-world behavior may differ
- Requires measurement infrastructure (dry-run migrations, monitoring)
- Post-copy approval process adds operational overhead

### Neutral

- Thresholds will be refined based on Phase 2 testing results
- Additional classes may be added as workload diversity increases

## Alternatives Considered

### Alternative 1: Single Policy for All VMs

Use one `MigrationPolicy` with conservative settings (high timeout, auto-converge enabled).

**Rejected:** Over-engineered for small VMs; masks performance issues; CPU throttling applied unnecessarily.

### Alternative 2: Per-VM Manual Tuning

Operators tune migration parameters manually per VM during maintenance.

**Rejected:** Not scalable; inconsistent; error-prone; no audit trail.

### Alternative 3: AI/ML-Based Classification

Use machine learning to predict migration behavior based on historical data.

**Deferred:** Requires data collection and model training; not feasible for initial implementation.

## Next Steps

1. Implement `MigrationPolicy` CRs for each profile
2. Label existing VMs according to classification matrix
3. Conduct dry-run migrations to validate threshold assumptions
4. Adjust thresholds based on observed migration metrics during Phase 2 testing
5. Document approval process for post-copy enablement

## References

- **[Migration Timeout Calculation](migration-timeout-calculation.md)** — Detailed methodology for calibrating `completionTimeoutPerGiB`
- [KubeVirt Live Migration Documentation](https://kubevirt.io/user-guide/operations/live_migration/)
- [OpenShift Virtualization: Configuring Live Migration Policies](https://docs.openshift.com/container-platform/latest/virt/virtual_machines/virt-node-maintenance-virt.html#virt-configuring-live-migration-policies_virt-node-maintenance-virt)
- [QEMU Migration: Pre-copy vs Post-copy](https://wiki.qemu.org/Features/PostCopyLiveMigration)
