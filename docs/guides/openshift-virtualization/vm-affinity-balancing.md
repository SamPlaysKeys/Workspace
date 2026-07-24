---
type: Guide
status: Active
system: OpenShift Virtualization
related_to: [KubeVirt, VM Scheduling, Affinity Rules]
references: []
---

# VM Affinity Balancing for Multi-Group Deployment

## Overview

This guide configures node affinity and pod anti-affinity to evenly distribute VMs across OpenShift nodes. The goal: ensure only **one VM from each group (GRP01, GRP02)** lands on each node, maximizing redundancy and preventing single points of failure.

## How It Works

| Mechanism | Purpose |
|-----------|---------|
| **Node Affinity** | Restricts which nodes a VM can schedule on (optional filtering) |
| **Pod Anti-Affinity** | Prevents multiple VMs from the same group on the same node |

## Prerequisites

- OpenShift cluster with OpenShift Virtualization operator installed
- Cluster-admin or namespace-admin access
- **4 nodes** in the cluster (required for 4 VMs per group with anti-affinity)
- VMs deployed via `VirtualMachine` CRD (KubeVirt)

## Target Architecture

| Group | VM Count | Distribution |
|-------|----------|--------------|
| GRP01 | 4 VMs | 1 per node |
| GRP02 | 4 VMs | 1 per node |
| **Total** | **8 VMs** | **2 per node** |

## Step 1: Label Your Nodes (Optional)

**Skip this step if you want VMs to schedule on all nodes in the cluster.**

Only label nodes if you want to restrict VMs to a specific subset of nodes:

```bash
# Example: Label specific nodes to restrict VM placement
oc label node <node1> node-role.kubernetes.io/vm-workload=""
oc label node <node2> node-role.kubernetes.io/vm-workload=""
oc label node <node3> node-role.kubernetes.io/vm-workload=""
oc label node <node4> node-role.kubernetes.io/vm-workload=""
```

### When to Use Node Labeling

**Example scenario:** Your cluster has 6 nodes, but only 4 have the larger storage volumes required for these VMs. Label those 4 nodes to ensure VMs only schedule where storage is available:

```
Cluster: 6 nodes total
├── node-1 (large storage)  ← labeled vm-workload
├── node-2 (large storage)  ← labeled vm-workload
├── node-3 (large storage)  ← labeled vm-workload
├── node-4 (large storage)  ← labeled vm-workload
├── node-5 (small storage)  ← not labeled
└── node-6 (small storage)  ← not labeled
```

Without labeling, the scheduler might place VMs on nodes 5-6, causing storage issues or pending pods.

**How balancing works without node labels:**

The anti-affinity rule uses `topologyKey: kubernetes.io/hostname`, which Kubernetes automatically sets to a unique value per node. This means each node is treated as a separate "zone" for scheduling purposes - no extra labels required.

## Step 2: Create VMs with Affinity Rules

> **Note:** If you skipped Step 1 (using all nodes), remove the `nodeAffinity` section from the YAML below. Only `podAntiAffinity` is required for balancing.

### GRP01 VMs (4 Total)

```yaml
apiVersion: kubevirt.io/v1
kind: VirtualMachine
metadata:
  name: grp01-vm-01
  namespace: your-namespace
  labels:
    vm-group: grp01
spec:
  running: false
  template:
    metadata:
      labels:
        vm-group: grp01
        kubevirt.io/domain: grp01-vm-01
    spec:
      affinity:
        nodeAffinity:
          requiredDuringSchedulingIgnoredDuringExecution:
            nodeSelectorTerms:
            - matchExpressions:
              - key: node-role.kubernetes.io/vm-workload
                operator: Exists
        podAntiAffinity:
          requiredDuringSchedulingIgnoredDuringExecution:
          - labelSelector:
              matchExpressions:
              - key: vm-group
                operator: In
                values:
                - grp01
            topologyKey: kubernetes.io/hostname
      domain:
        cpu:
          cores: 2
        memory:
          guest: 4Gi
        devices:
          disks:
          - disk:
              bus: virtio
            name: rootdisk
      volumes:
      - persistentVolumeClaim:
          claimName: grp01-vm-01-pvc
        name: rootdisk
---
apiVersion: kubevirt.io/v1
kind: VirtualMachine
metadata:
  name: grp01-vm-02
  namespace: your-namespace
  labels:
    vm-group: grp01
spec:
  running: false
  template:
    metadata:
      labels:
        vm-group: grp01
        kubevirt.io/domain: grp01-vm-02
    spec:
      affinity:
        nodeAffinity:
          requiredDuringSchedulingIgnoredDuringExecution:
            nodeSelectorTerms:
            - matchExpressions:
              - key: node-role.kubernetes.io/vm-workload
                operator: Exists
        podAntiAffinity:
          requiredDuringSchedulingIgnoredDuringExecution:
          - labelSelector:
              matchExpressions:
              - key: vm-group
                operator: In
                values:
                - grp01
            topologyKey: kubernetes.io/hostname
      domain:
        cpu:
          cores: 2
        memory:
          guest: 4Gi
        devices:
          disks:
          - disk:
              bus: virtio
            name: rootdisk
      volumes:
      - persistentVolumeClaim:
          claimName: grp01-vm-02-pvc
        name: rootdisk
---
apiVersion: kubevirt.io/v1
kind: VirtualMachine
metadata:
  name: grp01-vm-03
  namespace: your-namespace
  labels:
    vm-group: grp01
spec:
  running: false
  template:
    metadata:
      labels:
        vm-group: grp01
        kubevirt.io/domain: grp01-vm-03
    spec:
      affinity:
        nodeAffinity:
          requiredDuringSchedulingIgnoredDuringExecution:
            nodeSelectorTerms:
            - matchExpressions:
              - key: node-role.kubernetes.io/vm-workload
                operator: Exists
        podAntiAffinity:
          requiredDuringSchedulingIgnoredDuringExecution:
          - labelSelector:
              matchExpressions:
              - key: vm-group
                operator: In
                values:
                - grp01
            topologyKey: kubernetes.io/hostname
      domain:
        cpu:
          cores: 2
        memory:
          guest: 4Gi
        devices:
          disks:
          - disk:
              bus: virtio
            name: rootdisk
      volumes:
      - persistentVolumeClaim:
          claimName: grp01-vm-03-pvc
        name: rootdisk
---
apiVersion: kubevirt.io/v1
kind: VirtualMachine
metadata:
  name: grp01-vm-04
  namespace: your-namespace
  labels:
    vm-group: grp01
spec:
  running: false
  template:
    metadata:
      labels:
        vm-group: grp01
        kubevirt.io/domain: grp01-vm-04
    spec:
      affinity:
        nodeAffinity:
          requiredDuringSchedulingIgnoredDuringExecution:
            nodeSelectorTerms:
            - matchExpressions:
              - key: node-role.kubernetes.io/vm-workload
                operator: Exists
        podAntiAffinity:
          requiredDuringSchedulingIgnoredDuringExecution:
          - labelSelector:
              matchExpressions:
              - key: vm-group
                operator: In
                values:
                - grp01
            topologyKey: kubernetes.io/hostname
      domain:
        cpu:
          cores: 2
        memory:
          guest: 4Gi
        devices:
          disks:
          - disk:
              bus: virtio
            name: rootdisk
      volumes:
      - persistentVolumeClaim:
          claimName: grp01-vm-04-pvc
        name: rootdisk
```

### GRP02 VMs (4 Total)

```yaml
apiVersion: kubevirt.io/v1
kind: VirtualMachine
metadata:
  name: grp02-vm-01
  namespace: your-namespace
  labels:
    vm-group: grp02
spec:
  running: false
  template:
    metadata:
      labels:
        vm-group: grp02
        kubevirt.io/domain: grp02-vm-01
    spec:
      affinity:
        nodeAffinity:
          requiredDuringSchedulingIgnoredDuringExecution:
            nodeSelectorTerms:
            - matchExpressions:
              - key: node-role.kubernetes.io/vm-workload
                operator: Exists
        podAntiAffinity:
          requiredDuringSchedulingIgnoredDuringExecution:
          - labelSelector:
              matchExpressions:
              - key: vm-group
                operator: In
                values:
                - grp02
            topologyKey: kubernetes.io/hostname
      domain:
        cpu:
          cores: 2
        memory:
          guest: 4Gi
        devices:
          disks:
          - disk:
              bus: virtio
            name: rootdisk
      volumes:
      - persistentVolumeClaim:
          claimName: grp02-vm-01-pvc
        name: rootdisk
---
apiVersion: kubevirt.io/v1
kind: VirtualMachine
metadata:
  name: grp02-vm-02
  namespace: your-namespace
  labels:
    vm-group: grp02
spec:
  running: false
  template:
    metadata:
      labels:
        vm-group: grp02
        kubevirt.io/domain: grp02-vm-02
    spec:
      affinity:
        nodeAffinity:
          requiredDuringSchedulingIgnoredDuringExecution:
            nodeSelectorTerms:
            - matchExpressions:
              - key: node-role.kubernetes.io/vm-workload
                operator: Exists
        podAntiAffinity:
          requiredDuringSchedulingIgnoredDuringExecution:
          - labelSelector:
              matchExpressions:
              - key: vm-group
                operator: In
                values:
                - grp02
            topologyKey: kubernetes.io/hostname
      domain:
        cpu:
          cores: 2
        memory:
          guest: 4Gi
        devices:
          disks:
          - disk:
              bus: virtio
            name: rootdisk
      volumes:
      - persistentVolumeClaim:
          claimName: grp02-vm-02-pvc
        name: rootdisk
---
apiVersion: kubevirt.io/v1
kind: VirtualMachine
metadata:
  name: grp02-vm-03
  namespace: your-namespace
  labels:
    vm-group: grp02
spec:
  running: false
  template:
    metadata:
      labels:
        vm-group: grp02
        kubevirt.io/domain: grp02-vm-03
    spec:
      affinity:
        nodeAffinity:
          requiredDuringSchedulingIgnoredDuringExecution:
            nodeSelectorTerms:
            - matchExpressions:
              - key: node-role.kubernetes.io/vm-workload
                operator: Exists
        podAntiAffinity:
          requiredDuringSchedulingIgnoredDuringExecution:
          - labelSelector:
              matchExpressions:
              - key: vm-group
                operator: In
                values:
                - grp02
            topologyKey: kubernetes.io/hostname
      domain:
        cpu:
          cores: 2
        memory:
          guest: 4Gi
        devices:
          disks:
          - disk:
              bus: virtio
            name: rootdisk
      volumes:
      - persistentVolumeClaim:
          claimName: grp02-vm-03-pvc
        name: rootdisk
---
apiVersion: kubevirt.io/v1
kind: VirtualMachine
metadata:
  name: grp02-vm-04
  namespace: your-namespace
  labels:
    vm-group: grp02
spec:
  running: false
  template:
    metadata:
      labels:
        vm-group: grp02
        kubevirt.io/domain: grp02-vm-04
    spec:
      affinity:
        nodeAffinity:
          requiredDuringSchedulingIgnoredDuringExecution:
            nodeSelectorTerms:
            - matchExpressions:
              - key: node-role.kubernetes.io/vm-workload
                operator: Exists
        podAntiAffinity:
          requiredDuringSchedulingIgnoredDuringExecution:
          - labelSelector:
              matchExpressions:
              - key: vm-group
                operator: In
                values:
                - grp02
            topologyKey: kubernetes.io/hostname
      domain:
        cpu:
          cores: 2
        memory:
          guest: 4Gi
        devices:
          disks:
          - disk:
              bus: virtio
            name: rootdisk
      volumes:
      - persistentVolumeClaim:
          claimName: grp02-vm-04-pvc
        name: rootdisk
```

## Step 3: Deploy and Start VMs

```bash
# Apply VM definitions
oc apply -f grp01-vms.yaml
oc apply -f grp02-vms.yaml

# Start all GRP01 VMs
oc patch vm grp01-vm-01 --type merge -p '{"spec":{"running":true}}'
oc patch vm grp01-vm-02 --type merge -p '{"spec":{"running":true}}'
oc patch vm grp01-vm-03 --type merge -p '{"spec":{"running":true}}'
oc patch vm grp01-vm-04 --type merge -p '{"spec":{"running":true}}'

# Start all GRP02 VMs
oc patch vm grp02-vm-01 --type merge -p '{"spec":{"running":true}}'
oc patch vm grp02-vm-02 --type merge -p '{"spec":{"running":true}}'
oc patch vm grp02-vm-03 --type merge -p '{"spec":{"running":true}}'
oc patch vm grp02-vm-04 --type merge -p '{"spec":{"running":true}}'
```

## Step 4: Verify Distribution

```bash
# Check which node each VMI (VirtualMachineInstance) is running on
oc get vmi -o custom-columns=NAME:.metadata.name,NODE:.status.nodeName,GROUP:.metadata.labels.vm-group

# Expected output (4 nodes, 8 VMs):
# NAME          NODE          GROUP
# grp01-vm-01   node-1        grp01
# grp01-vm-02   node-2        grp01
# grp01-vm-03   node-3        grp01
# grp01-vm-04   node-4        grp01
# grp02-vm-01   node-1        grp02
# grp02-vm-02   node-2        grp02
# grp02-vm-03   node-3        grp02
# grp02-vm-04   node-4        grp02

# Check pod placement directly
oc get pods -l kubevirt.io/domain -o wide
```

## How Anti-Affinity Enforces Balance

```
Node-1              Node-2              Node-3              Node-4
-------             -------             -------             -------
grp01-vm-01         grp01-vm-02         grp01-vm-03         grp01-vm-04
grp02-vm-01         grp02-vm-02         grp02-vm-03         grp02-vm-04
```

Each group's anti-affinity rule prevents a second VM from that group landing on an already-occupied node. Result: **one GRP01 VM + one GRP02 VM per node**.

## Required vs Preferred Affinity

| Type | Behavior | Use Case |
|------|----------|----------|
| `requiredDuringSchedulingIgnoredDuringExecution` | Hard constraint - pod stays pending if no valid node | Critical HA requirements |
| `preferredDuringSchedulingIgnoredDuringExecution` | Soft constraint - best effort scheduling | Flexible distribution |

For strict "one per group per node" enforcement, use **required**.

## Common Pitfalls

| Issue | Cause | Fix |
|-------|-------|-----|
| VM stuck in `Pending` | Not enough nodes to satisfy anti-affinity | Add nodes or reduce VM count per group |
| All VMs on one node | Missing or incorrect label selector | Verify `vm-group` label matches in VM spec and selector |
| Anti-affinity ignored | Used `preferred` instead of `required` | Use `requiredDuringSchedulingIgnoredDuringExecution` |
| Node affinity blocks scheduling | Node label missing or typo'd | Verify labels with `oc get nodes --show-labels` |

## Scaling Guidance

| VMs per Group | Minimum Nodes Required |
|---------------|------------------------|
| 1 | 1 (no anti-affinity effect) |
| 2 | 2 |
| 3 | 3 |
| **4** | **4** (this guide) |
| N | N |

## Quick One-Liner for Verification

```bash
oc get vmi -L vm-group -o custom-columns=NAME:.metadata.name,NODE:.status.nodeName,GROUP:.metadata.labels.vm-group | column -t
```

## References

- [Kubernetes Affinity Documentation](https://kubernetes.io/docs/concepts/scheduling-eviction/assign-pod-node/#affinity-and-anti-affinity)
- [KubeVirt VM Scheduling](https://kubevirt.io/user-guide/virtual_machines/scheduling/)
