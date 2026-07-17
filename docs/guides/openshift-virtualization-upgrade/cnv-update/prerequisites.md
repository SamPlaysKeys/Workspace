---
type: Guide
---
# CNV Update Prerequisites

Version compatibility, channel selection, and pre-update verification.

## Version Compatibility

OpenShift Virtualization version must be compatible with the OpenShift Container Platform version.

### General Rule

| OpenShift Version | CNV Version | Channel |
|-------------------|-------------|---------|
| 4.15 | 4.15.x | `stable-4.15` |
| 4.16 | 4.16.x | `stable-4.16` |
| 4.17 | 4.17.x | `stable-4.17` |
| 4.18 | 4.18.x | `stable-4.18` |

> **Note:** CNV versions align with OpenShift minor versions. CNV 4.17 runs on OpenShift 4.17.

### Cross-Version Compatibility

Some CNV versions support multiple OpenShift versions. Check the CNV release notes for specific compatibility matrix.

## Channel Selection

### Stable Channel

**Recommended for most environments.**

```yaml
apiVersion: operators.coreos.com/v1alpha1
kind: Subscription
metadata:
  name: kubevirt-hyperconverged
  namespace: openshift-cnv
spec:
  channel: stable
  name: kubevirt-hyperconverged
  source: redhat-operators
  sourceNamespace: openshift-marketplace
```

**Behavior:**
- Automatically updates to the latest z-stream for current minor version
- When new minor version is available and compatible, prompts for approval (if `Automatic` approval)

### Stable-x.y Channel

**Pinned to specific minor version.**

```yaml
spec:
  channel: stable-4.17
```

**Behavior:**
- Updates only within 4.17.z stream
- Does not prompt for minor version upgrade
- Useful for controlled upgrade cycles

### Approval Strategy

| Strategy | Behavior | Use Case |
|----------|----------|----------|
| `Automatic` | Installs updates without approval | Test/dev environments |
| `Manual` | Requires approval for each update | Production environments |

```yaml
spec:
  installPlanApproval: Manual  # or Automatic
```

## Pre-Update Checks

### 1. Check Current CNV Version

```bash
oc get csv -n openshift-cnv | grep kubevirt-hyperconverged
```

### 2. Check HyperConverged Operator Status

```bash
oc get hco kubevirt-hyperconverged -n openshift-cnv -o json | jq '.status.conditions[] | select(.type == "Upgradeable")'
```

Expected: `"status": "True"`

### 3. Check Available Updates

```bash
oc get packagemanifest kubevirt-hyperconverged -n openshift-marketplace -o json | jq '.status.channels[] | select(.name == "stable") | .currentCSV'
```

### 4. Check VM Count and Status

```bash
# Total VMs
oc get vm -A --no-headers | wc -l

# Running VMs
oc get vmi -A --no-headers | wc -l

# VMs with live migration enabled
oc get vm -A -o json | jq '.items[] | select(.spec.template.spec.evictionStrategy == "LiveMigrate") | .metadata.name'
```

### 5. Check Migration Policies

```bash
oc get migrationpolicy -A
```

### 6. Check Node Capacity

```bash
oc get nodes -o custom-columns=NAME:.metadata.name,CPU:.status.capacity.cpu,MEMORY:.status.capacity.memory
```

### 7. Check for Active Migrations

```bash
oc get vmim -A
```

If migrations are in progress, wait for completion before updating.

## Update Timing Considerations

| Factor | Recommendation |
|--------|----------------|
| **Maintenance window** | Schedule during low-activity period |
| **Running VMs** | Verify capacity for migrations |
| **Storage** | Ensure PVCs are healthy |
| **Network** | Confirm migration network is stable |

## Subscription Configuration Example

Complete subscription with recommended settings:

```yaml
apiVersion: operators.coreos.com/v1alpha1
kind: Subscription
metadata:
  name: kubevirt-hyperconverged
  namespace: openshift-cnv
spec:
  channel: stable
  name: kubevirt-hyperconverged
  source: redhat-operators
  sourceNamespace: openshift-marketplace
  installPlanApproval: Manual
  startingCSV: kubevirt-hyperconverged-operator.v4.17.0  # Optional: specific version
```

## Next Steps

After verifying prerequisites:

1. **Configure HyperConverged CR** — Set `workloadUpdateStrategy` (see [hyperconverged-config.md](hyperconverged-config.md))
2. **Approve update** — If `Manual` approval, approve the install plan
3. **Monitor progress** — Watch upgrade status and VM migrations (see [monitoring-upgrade.md](monitoring-upgrade.md))

## Red Hat Documentation

- [OpenShift Virtualization: Updating OpenShift Virtualization](https://docs.redhat.com/en/documentation/openshift_container_platform/latest/html/virtualization/updating) — Official update documentation
- [Installing OpenShift Virtualization](https://docs.openshift.com/container-platform/latest/virt/install/virt-installing-virt-operator.html) — Operator installation and subscription configuration
- [Operator Lifecycle Manager: Subscription configuration](https://docs.openshift.com/container-platform/latest/operators/understanding/olm/olm-understanding-olm.html#olm-subscriptions_olm-understanding-olm) — Subscription CR fields and behavior
- [Cluster version compatibility](https://docs.openshift.com/container-platform/latest/updating/updating_a_cluster/updating-cluster-cli.html) — OpenShift upgrade paths and channels
