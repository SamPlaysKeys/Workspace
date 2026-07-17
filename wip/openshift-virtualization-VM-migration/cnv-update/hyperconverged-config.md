---
type: Design
---
# HyperConverged CR Configuration for CNV Updates

Configure `workloadUpdateStrategy` to control how VM workloads are updated during CNV operator upgrades.

## How CNV Updates Affect VMs

When OpenShift Virtualization is updated, the operator updates VM workloads automatically:

| Component | Update Method |
|-----------|---------------|
| `virt-launcher` pods | Live migration to new pods with updated image |
| `libvirt` | Updated via new `virt-launcher` pod |
| `qemu` | Updated via new `virt-launcher` pod |

**Key requirement:** VMs must support live migration to be updated automatically.

## Workload Update Strategy

The `workloadUpdateStrategy` stanza in the `HyperConverged` CR controls how VMs are handled during updates.

### Update Methods

| Method | Behavior | Use When |
|--------|----------|----------|
| `LiveMigrate` | Migrate running VMs to new pods | Default, safe for running workloads |
| `Evict` | Stop VM and restart on new pod | Fallback for VMs that can't migrate |

**Recommended:** Configure both methods — `LiveMigrate` as primary, `Evict` as fallback.

### Batch Settings

Control how many VMs are updated simultaneously:

| Setting | Description | Default |
|---------|-------------|---------|
| `batchEvictionSize` | Number of VMs to evict per batch | 10 |
| `batchEvictionInterval` | Wait time between batches | 1 minute |

Tune these based on:
- Number of running VMs
- Available cluster capacity
- Maintenance window duration
- Risk tolerance

## Configuration Profiles

### Profile 1: Baseline (Recommended)

Balanced approach for most environments.

```yaml
apiVersion: hco.kubevirt.io/v1beta1
kind: HyperConverged
metadata:
  name: kubevirt-hyperconverged
  namespace: openshift-cnv
spec:
  virtualization:
    workloadUpdateStrategy:
      workloadUpdateMethods:
      - LiveMigrate
      - Evict
      batchEvictionSize: 10
      batchEvictionInterval: "1m0s"
```

**Characteristics:**
- Live migrate VMs that support it
- Evict VMs that can't migrate (will restart)
- Process 10 VMs at a time
- 1 minute between batches

### Profile 2: Aggressive

Fast updates for maintenance windows with limited time.

```yaml
spec:
  virtualization:
    workloadUpdateStrategy:
      workloadUpdateMethods:
      - LiveMigrate
      - Evict
      batchEvictionSize: 20
      batchEvictionInterval: "30s"
```

**Characteristics:**
- Larger batches (20 VMs)
- Shorter interval (30 seconds)
- Higher risk of resource contention
- Faster completion

**Use when:**
- Tight maintenance windows
- Sufficient cluster capacity
- Non-production workloads

### Profile 3: Conservative

Cautious approach for critical workloads.

```yaml
spec:
  virtualization:
    workloadUpdateStrategy:
      workloadUpdateMethods:
      - LiveMigrate
      batchEvictionSize: 5
      batchEvictionInterval: "2m0s"
```

**Characteristics:**
- Live migrate only (no Evict)
- Smaller batches (5 VMs)
- Longer interval (2 minutes)
- VMs without live migration support are **not updated**

**Use when:**
- Critical workloads
- No VM restarts allowed
- Sufficient maintenance window

## Integration with MigrationPolicy

The `workloadUpdateStrategy` works with `MigrationPolicy` CRs:

| Layer | Scope | Purpose |
|-------|-------|---------|
| `HyperConverged.workloadUpdateStrategy` | Cluster-wide | Controls update method and batching |
| `MigrationPolicy` | Per-VM class | Controls migration parameters (timeout, bandwidth, auto-converge) |

**Recommendation:** Configure `MigrationPolicy` for different VM classes (see `../adr/0001-vm-policy-thresholds.md`) and use `workloadUpdateStrategy` for cluster-wide batching.

## Live Migration Configuration

Also configure live migration settings in the `HyperConverged` CR:

```yaml
spec:
  virtualization:
    liveMigrationConfig:
      completionTimeoutPerGiB: 5        # Seconds per GiB of memory
      parallelMigrationsPerCluster: 5   # Max concurrent migrations
      parallelOutboundMigrationsPerNode: 2  # Max migrations per node
      bandwidthPerMigration: "0"        # 0 = unlimited
```

> **Note:** See `../migration-timeout-calculation.md` for timeout calibration guidance.

## Eviction Strategy on VMs

For a VM to be live-migrated during update, it must have the `LiveMigrate` eviction strategy:

```yaml
apiVersion: kubevirt.io/v1
kind: VirtualMachine
metadata:
  name: my-vm
spec:
  template:
    spec:
      evictionStrategy: LiveMigrate
```

Check existing VMs:

```bash
oc get vm -A -o json | jq '.items[] | select(.spec.template.spec.evictionStrategy == "LiveMigrate") | .metadata.name'
```

VMs without this setting will use the `Evict` method (if configured) or will not be updated during the CNV upgrade.

## Patching HyperConverged CR

Apply configuration without editing the full CR:

```bash
oc patch -n openshift-cnv hco kubevirt-hyperconverged \
  --type=merge \
  -p '{"spec": {"virtualization": {"workloadUpdateStrategy": {"workloadUpdateMethods": ["LiveMigrate","Evict"], "batchEvictionSize": 10, "batchEvictionInterval": "1m0s"}}}}'
```

## Decision Matrix

| Scenario | Profile | Rationale |
|----------|---------|-----------|
| Standard production | Baseline | Balanced speed and safety |
| Tight maintenance window | Aggressive | Faster completion, higher capacity needed |
| Critical workloads | Conservative | No Evict, smaller batches |
| All VMs support live migration | Baseline or Aggressive | Evict fallback rarely used |
| Some VMs don't support live migration | Baseline | Evict allows update completion |

## Artifacts

See `artifacts/hyperconverged/` for complete YAML files:
- `baseline-workload-update.yml`
- `aggressive-workload-update.yml`
- `conservative-workload-update.yml`

## Red Hat Documentation

- [Configuring workload update methods](https://docs.openshift.com/container-platform/latest/virt/virtual_machines/virt-node-maintenance-virt.html#virt-configuring-workload-update-methods_virt-node-maintenance-virt) — Official HyperConverged CR configuration guide
- [Live migration configuration](https://docs.openshift.com/container-platform/latest/virt/live_migration/virt-configuring-live-migration.html) — Live migration settings in HyperConverged CR
- [VM eviction strategies](https://docs.openshift.com/container-platform/latest/virt/virtual_machines/virt-node-maintenance-virt.html) — Configuring VMs for live migration during updates
- [HyperConverged CR reference](https://docs.openshift.com/container-platform/latest/virt/install/virt-installing-virt-operator.html#virt-hyperconverged-cr_virt-installing-virt-operator) — CR specification
