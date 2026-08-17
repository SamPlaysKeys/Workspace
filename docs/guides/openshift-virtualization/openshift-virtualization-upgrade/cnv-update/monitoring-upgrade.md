---
type: Guide
---
# Monitoring CNV Upgrade Progress

Monitor operator status, VM migrations, and verify all workloads are updated.

## Upgrade Phases

1. **Subscription update** — OLM pulls new CSV
2. **Operator update** — CNV operator pods restart
3. **Workload update** — VMs live-migrated to new `virt-launcher` pods
4. **Completion** — All components updated, no outdated workloads

## Monitoring Commands

### 1. Check Subscription Status

```bash
oc get subscription kubevirt-hyperconverged -n openshift-cnv -o yaml
```

Key fields:
- `status.state` — Should be `AtLatestKnown` when complete
- `status.currentCSV` — The installed version
- `status.installPlanRef` — Pending install plan if `Manual` approval

### 2. Check Install Plan

```bash
oc get installplan -n openshift-cnv
```

If `Manual` approval:
```bash
oc get installplan -n openshift-cnv -o yaml | grep -A 5 approval
```

Approve pending install:
```bash
oc patch installplan <install-plan-name> -n openshift-cnv --type=merge -p '{"spec":{"approved":true}}'
```

### 3. Check CSV Status

```bash
oc get csv -n openshift-cnv | grep kubevirt-hyperconverged
```

Phases:
- `Installing` — Operator is being deployed
- `Succeeded` — Operator is ready
- `Failed` — Check events and logs

### 4. Check HyperConverged Operator Status

```bash
oc get hco kubevirt-hyperconverged -n openshift-cnv -o json | jq '.status.conditions'
```

Key conditions:
- `Available` — Operator is running
- `Progressing` — Update in progress
- `Degraded` — Error condition
- `Upgradeable` — Safe to upgrade

### 5. Check for Outdated Workloads

After operator update completes, check for VMs with outdated `virt-launcher` pods:

```bash
oc get kv kubevirt-kubevirt-hyperconverged -n openshift-cnv -o json | jq '.status.outdatedVirtualMachineInstanceWorkloads'
```

- `0` — All VMs updated
- `> 0` — VMs pending update

List specific VMIs:
```bash
oc get vmi -l kubevirt.io/outdatedLauncherImage --all-namespaces
```

### 6. Check Active Migrations

```bash
oc get vmim -A
```

Monitor specific migration:
```bash
oc get vmim <migration-name> -n <namespace> -o yaml
```

Migration phases:
- `Pending` — Waiting for resources
- `Running` — Migration in progress
- `Succeeded` — Complete
- `Failed` — Check migration status for error

### 7. Check virt-launcher Pod Versions

```bash
oc get pods -n openshift-cnv -l kubevirt.io=virt-launcher -o json | jq '.items[] | {name: .metadata.name, image: .spec.containers[0].image}'
```

Compare with expected version from CSV.

## Verification Checklist

| Check | Command | Expected Result |
|-------|---------|-----------------|
| Subscription state | `oc get subscription -n openshift-cnv` | `AtLatestKnown` |
| CSV phase | `oc get csv -n openshift-cnv` | `Succeeded` |
| HCO conditions | `oc get hco -n openshift-cnv -o json \| jq .status.conditions` | `Available: True`, `Progressing: False` |
| Outdated workloads | `oc get kv -n openshift-cnv -o json \| jq .status.outdatedVirtualMachineInstanceWorkloads` | `0` |
| Active migrations | `oc get vmim -A` | No migrations running |
| VM status | `oc get vmi -A` | All VMs running |

## Troubleshooting

### Stuck Install Plan

```bash
oc describe installplan <name> -n openshift-cnv
```

Check for:
- Missing approval
- Resource conflicts
- Image pull errors

### Migrations Not Starting

Verify:
1. VM has `evictionStrategy: LiveMigrate`
2. `MigrationPolicy` allows migration
3. Target node has capacity
4. No ongoing migrations consuming resources

### Migrations Stuck in Pending

```bash
oc describe vmim <name> -n <namespace>
```

Common causes:
- Insufficient cluster resources
- Node selector constraints
- PVC access mode issues
- Network issues

### Outdated Workloads Not Decreasing

1. Check migration status:
   ```bash
   oc get vmim -A
   ```

2. Check for VMs without live migration support:
   ```bash
   oc get vm -A -o json | jq '.items[] | select(.spec.template.spec.evictionStrategy != "LiveMigrate") | .metadata.name'
   ```

3. If `Evict` method is enabled, VMs should restart automatically

4. If `Evict` is not enabled, manually restart VMs:
   ```bash
   virtctl restart <vm-name> -n <namespace>
   ```

## Monitoring with Prometheus

CNV exposes metrics for migration monitoring:

```promql
# Active migrations
kubevirt_vmi_migrations_in_progress

# Migration duration
kubevirt_vmi_migration_succeeded_total

# Migration failures
kubevirt_vmi_migration_failed_total
```

Create an alert for outdated workloads:
```yaml
- alert: OutdatedVirtLauncherWorkloads
  expr: kubevirt_outdated_virt_launcher_workloads > 0
  for: 30m
  labels:
    severity: warning
  annotations:
    summary: "VM workloads have outdated virt-launcher pods"
    description: "{{ $value }} VMIs are running with outdated virt-launcher images"
```

## Artifacts

See `artifacts/verification/` for:
- `check-outdated-workloads.sh` — Shell script for status checks
- `monitor-upgrade-progress.yml` — Ansible playbook for automated monitoring

## Red Hat Documentation

- [OpenShift Virtualization: Updating OpenShift Virtualization](https://docs.redhat.com/en/documentation/openshift_container_platform/latest/html/virtualization/updating) — Official update documentation including monitoring steps
- [Monitoring live migrations](https://docs.openshift.com/container-platform/latest/virt/live_migration/virt-monitoring-live-migration.html) — Migration status and troubleshooting
- [Operator Lifecycle Manager: Managing operators](https://docs.openshift.com/container-platform/latest/operators/admin/olm-managing-operators.html) — CSV, Subscription, and InstallPlan management
- [OpenShift monitoring](https://docs.openshift.com/container-platform/latest/monitoring/monitoring-overview.html) — Prometheus metrics and alerting
