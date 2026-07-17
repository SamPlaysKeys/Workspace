---
type: Guide
---
# Control Plane Only Updates (OpenShift 4.16+)

Special handling for OpenShift 4.16+ control-plane-only updates that require CNV workload updates to be disabled.

## Background: What Changed in OpenShift 4.16

Starting with OpenShift 4.16:

- RHCOS upgraded to version 9.4
- All `virt-launcher` pods must use the same RHCOS version
- During control-plane-only update, CNV workload updates are **automatically disabled**
- This prevents `virt-launcher` pods from updating before control plane is stable

## Control Plane Only Update Flow

### Standard OpenShift Upgrade Sequence

```
OpenShift 4.15 → 4.16:

1. Control plane nodes upgrade (masters)
   - CNV workload updates DISABLED during this phase
   - Existing virt-launcher pods continue running
   - No VM migrations triggered by CNV

2. Control plane upgrade completes
   - All master nodes on RHCOS 9.4
   - API server stable
   - etcd healthy

3. Re-enable CNV workload updates
   - Manual step required
   - Triggers virt-launcher pod updates

4. Wait for VM workloads to update
   - VMs live-migrated to new pods
   - All virt-launcher pods use RHCOS 9.4

5. Worker nodes upgrade
   - CNV handles VM migrations normally
   - workloadUpdateStrategy applies

6. Upgrade complete
```

## Step-by-Step Process

### Step 1: Before Control Plane Upgrade

Verify current state:

```bash
# Check OpenShift version
oc get clusterversion

# Check CNV version
oc get csv -n openshift-cnv | grep kubevirt-hyperconverged

# Check outdated workloads (should be 0 before upgrade)
oc get kv kubevirt-kubevirt-hyperconverged -n openshift-cnv -o json | jq '.status.outdatedVirtualMachineInstanceWorkloads'
```

### Step 2: Initiate Control Plane Upgrade

```bash
# Update channel if needed
oc patch clusterversion version --type=merge -p '{"spec":{"channel":"stable-4.16"}}'

# Trigger update
oc adm upgrade --to=4.16.x
```

### Step 3: Monitor Control Plane Upgrade

During control plane upgrade:
- CNV operator remains running
- VMs continue running on existing pods
- **Do not** attempt to trigger VM migrations manually

```bash
# Monitor control plane nodes
oc get nodes

# Monitor cluster version
oc get clusterversion

# Monitor CNV operator (should remain Available)
oc get hco kubevirt-hyperconverged -n openshift-cnv -o json | jq '.status.conditions[] | select(.type == "Available")'
```

### Step 4: Verify Control Plane Complete

```bash
# All control plane nodes Ready
oc get nodes -l node-role.kubernetes.io/control-plane

# Cluster version updated
oc get clusterversion

# Check for control-plane-only condition
oc get clusterversion -o json | jq '.status.conditions[] | select(.type == "ControlPlaneOnly")'
```

### Step 5: Re-enable CNV Workload Updates

After control plane upgrade completes:

```bash
# Check if workload updates are disabled
oc get kv kubevirt-kubevirt-hyperconverged -n openshift-cnv -o json | jq '.spec.workloadUpdateStrategy'
```

If `workloadUpdateMethods` is empty or missing, re-enable:

```bash
oc patch -n openshift-cnv hco kubevirt-hyperconverged \
  --type=merge \
  -p '{"spec": {"virtualization": {"workloadUpdateStrategy": {"workloadUpdateMethods": ["LiveMigrate","Evict"]}}}}'
```

### Step 6: Monitor Workload Updates

```bash
# Check outdated workloads count
oc get kv kubevirt-kubevirt-hyperconverged -n openshift-cnv -o json | jq '.status.outdatedVirtualMachineInstanceWorkloads'

# Watch VMIs with outdated launcher
oc get vmi -l kubevirt.io/outdatedLauncherImage --all-namespaces -w

# Monitor migrations
oc get vmim -A -w
```

### Step 7: Verify All Workloads Updated

```bash
# Outdated count should be 0
oc get kv kubevirt-kubevirt-hyperconverged -n openshift-cnv -o json | jq '.status.outdatedVirtualMachineInstanceWorkloads'

# No VMIs with outdated label
oc get vmi -l kubevirt.io/outdatedLauncherImage --all-namespaces
```

Expected: No resources found.

### Step 8: Proceed with Worker Node Upgrade

After verifying all CNV workloads are updated:

```bash
# Trigger worker node upgrade
oc adm upgrade --to=4.16.x
```

CNV will handle VM migrations during worker node drains per the configured `workloadUpdateStrategy`.

## Key Considerations

| Phase | CNV Workload Updates | VM Behavior |
|-------|---------------------|-------------|
| Before control plane upgrade | Normal | Can migrate if needed |
| During control plane upgrade | **Disabled** | Continue running, no automatic migration |
| After control plane upgrade (re-enabled) | Triggered | VMs migrate to updated pods |
| During worker node upgrade | Normal | Live migrate during node drain |

## Verification Commands

Complete verification script:

```bash
#!/bin/bash
# Verify CNV workloads are ready after control plane upgrade

echo "=== OpenShift Version ==="
oc get clusterversion

echo -e "\n=== Control Plane Nodes ==="
oc get nodes -l node-role.kubernetes.io/control-plane

echo -e "\n=== CNV Operator Status ==="
oc get hco kubevirt-hyperconverged -n openshift-cnv -o json | jq '.status.conditions[] | {type: .type, status: .status}'

echo -e "\n=== Outdated Workloads ==="
OUTDATED=$(oc get kv kubevirt-kubevirt-hyperconverged -n openshift-cnv -o json | jq '.status.outdatedVirtualMachineInstanceWorkloads')
echo "Count: $OUTDATED"

if [ "$OUTDATED" != "0" ]; then
  echo -e "\n=== VMIs with Outdated Launcher ==="
  oc get vmi -l kubevirt.io/outdatedLauncherImage --all-namespaces
fi

echo -e "\n=== Active Migrations ==="
oc get vmim -A
```

## Common Issues

### Issue: Workload Updates Not Re-enabled

**Symptom:** `outdatedVirtualMachineInstanceWorkloads` stays > 0 after control plane upgrade.

**Resolution:**
```bash
# Check if workloadUpdateMethods is configured
oc get kv kubevirt-kubevirt-hyperconverged -n openshift-cnv -o json | jq '.spec.workloadUpdateStrategy'

# If empty or missing, patch it
oc patch -n openshift-cnv hco kubevirt-hyperconverged \
  --type=merge \
  -p '{"spec": {"virtualization": {"workloadUpdateStrategy": {"workloadUpdateMethods": ["LiveMigrate","Evict"]}}}}'
```

### Issue: VMs Stuck with Outdated Launcher

**Symptom:** VMIs with `kubevirt.io/outdatedLauncherImage` label remain after re-enabling updates.

**Causes:**
1. VM doesn't support live migration
2. Migration policy too restrictive
3. Insufficient resources on target nodes
4. `Evict` method not enabled and VM can't migrate

**Resolution:**
```bash
# Check VMI details
oc describe vmi <name> -n <namespace>

# If VM can't migrate, restart manually
virtctl restart <vm-name> -n <namespace>

# Or enable Evict method if appropriate
```

### Issue: Proceeding to Next Upgrade Before Workloads Updated

**Symptom:** Attempting to upgrade to 4.17 before 4.16 workload updates complete.

**Risk:** `virt-launcher` pods on mixed RHCOS versions can cause instability.

**Prevention:** Always verify `outdatedVirtualMachineInstanceWorkloads = 0` before initiating next upgrade.

## Red Hat Documentation

- [Preventing workload updates during a control plane only update](https://docs.openshift.com/container-platform/latest/virt/virtual_machines/virt-node-maintenance-virt.html#virt-preventing-workload-updates-during-control-plane-only-update_virt-node-maintenance-virt) — Official documentation for OpenShift 4.16+ behavior
- [OpenShift Virtualization: Updating OpenShift Virtualization](https://docs.redhat.com/en/documentation/openshift_container_platform/latest/html/virtualization/updating) — Complete update documentation
- [Understanding OpenShift updates](https://docs.openshift.com/container-platform/latest/updating/understanding_updates/understanding-updates.html) — Control plane vs worker node upgrade phases
- [Updating a cluster](https://docs.openshift.com/container-platform/latest/updating/updating_a_cluster/updating-cluster-cli.html) — Cluster upgrade process and commands

## Related Documents

- `../prerequisites.md` — Pre-update verification
- `../monitoring-upgrade.md` — Monitoring commands
