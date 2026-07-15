---
type: Reference
---

# Reference Document: Ansible Core Path

This document provides CRD schemas, Ansible module references, and links to external documentation for the Ansible Core execution path.

---

## Core CRDs (Custom Resource Definitions)

### VirtualMachineInstanceMigration (kubevirt.io/v1)

Triggers a live migration of a running VirtualMachineInstance (VMI) to another node.

```yaml
apiVersion: kubevirt.io/v1
kind: VirtualMachineInstanceMigration
metadata:
  name: migrate-my-vm
  namespace: my-namespace
spec:
  vmiName: my-vm-instance
```

**Status Fields:**
- `status.phase` — Migration phase (`Pending`, `Scheduling`, `Scheduled`, `PreparingTarget`, `TargetReady`, `Running`, `Succeeded`, `Failed`)
- `status.migrationState.sourceNode` — Source node name
- `status.migrationState.targetNode` — Target node name
- `status.migrationState.startTimestamp` — Migration start time

**Reference:** https://kubevirt.io/api-reference/master/definitions.html#_v1_virtualmachineinstancemigration

---

### MigrationPolicy (kubevirt.io/v1alpha1)

Cluster-scoped policy that controls migration behavior for VMs matching a label selector.

```yaml
apiVersion: kubevirt.io/v1alpha1
kind: MigrationPolicy
metadata:
  name: large-memory-policy
spec:
  selectors:
    virtualMachineInstanceSelector:
      migration-policy: large-memory
  allowPostCopy: false
  completionTimeoutPerGiB: 800
  parallelMigrationsPerCluster: 5
  parallelOutboundMigrationsPerNode: 2
  bandwidthPerMigration: "64Mi"
  autoConverge: false
```

**Key Spec Fields:**
| Field | Type | Description |
|-------|------|-------------|
| `allowPostCopy` | boolean | Enable post-copy migration for VMs matching this policy |
| `completionTimeoutPerGiB` | integer | Timeout in seconds per GiB of guest memory |
| `parallelMigrationsPerCluster` | integer | Maximum parallel migrations across cluster |
| `parallelOutboundMigrationsPerNode` | integer | Maximum outbound migrations per node |
| `bandwidthPerMigration` | string | Bandwidth limit per migration (e.g., "64Mi") |
| `autoConverge` | boolean | Enable auto-converge to throttle guest CPU |

**Reference:** https://docs.openshift.com/container-platform/latest/virt/virtual_machines/virt-node-maintenance-virt.html#virt-configuring-live-migration-policies_virt-node-maintenance-virt

---

### VirtualMachineInstance (kubevirt.io/v1)

Represents a running VM instance. Check VMI status to determine current node.

```yaml
apiVersion: kubevirt.io/v1
kind: VirtualMachineInstance
metadata:
  name: my-vm-instance
  namespace: my-namespace
spec:
  migrationPolicy: large-memory-policy  # Optional: attach policy by name
status:
  nodeName: worker-01  # Current node
  migrationState:      # Present during migration
    sourceNode: worker-01
    targetNode: worker-02
    phase: Running
```

**Reference:** https://kubevirt.io/api-reference/master/definitions.html#_v1_virtualmachineinstance

---

## Ansible Modules

### kubernetes.core.k8s

Generic module for managing Kubernetes/OpenShift resources.

```yaml
- name: Create VirtualMachineInstanceMigration
  kubernetes.core.k8s:
    state: present
    definition:
      apiVersion: kubevirt.io/v1
      kind: VirtualMachineInstanceMigration
      metadata:
        name: "{{ migration_name }}"
        namespace: "{{ namespace }}"
      spec:
        vmiName: "{{ vmi_name }}"
```

**Documentation:** https://docs.ansible.com/ansible/latest/collections/kubernetes/core/k8s_module.html

---

### kubernetes.core.k8s_info

Module for querying Kubernetes/OpenShift resources.

```yaml
- name: Get all VMIs
  kubernetes.core.k8s_info:
    api_version: kubevirt.io/v1
    kind: VirtualMachineInstance
    namespace: "{{ namespace }}"
  register: vmi_list

- name: Get VMI on specific node
  kubernetes.core.k8s_info:
    api_version: kubevirt.io/v1
    kind: VirtualMachineInstance
    namespace: "{{ namespace }}"
    label_selectors:
      - "kubevirt.io/nodeName={{ target_node }}"
  register: vmi_on_node
```

**Documentation:** https://docs.ansible.com/ansible/latest/collections/kubernetes/core/k8s_info_module.html

---

### kubernetes.core.k8s_drain

Module for draining and cordoning nodes.

```yaml
- name: Cordon node
  kubernetes.core.k8s_drain:
    name: "{{ node_name }}"
    state: cordon

- name: Drain node
  kubernetes.core.k8s_drain:
    name: "{{ node_name }}"
    state: drain
    delete_options:
      ignore_daemonsets: true
      delete_emptydir_data: true
      force: true

- name: Uncordon node
  kubernetes.core.k8s_drain:
    name: "{{ node_name }}"
    state: uncordon
```

**Documentation:** https://docs.ansible.com/ansible/latest/collections/kubernetes/core/k8s_drain_module.html

---

## OpenShift CLI Commands

### Node Management

```bash
# List nodes
oc get nodes

# Cordon node (mark unschedulable)
oc adm cordon <node-name>

# Uncordon node (mark schedulable)
oc adm uncordon <node-name>

# Drain node
oc adm drain <node-name> --ignore-daemonsets --delete-emptydir-data --force

# Get node details
oc describe node <node-name>
```

### VM/VMI Management

```bash
# List VMIs with node placement
oc get vmi -o wide

# Get VMI details
oc describe vmi <vmi-name> -n <namespace>

# Trigger migration via CLI
virtctl migrate <vm-name> -n <namespace>

# Cancel migration
oc delete vmim <migration-name> -n <namespace>

# Check migration status
oc get vmim <migration-name> -n <namespace> -o yaml
```

### Migration Policy

```bash
# List migration policies
oc get migrationpolicy -n openshift-cnv

# Get policy details
oc describe migrationpolicy <policy-name> -n openshift-cnv

# Create/update policy
oc apply -f migration-policy.yaml
```

---

## External Documentation Links

### OpenShift Virtualization

| Topic | Link |
|-------|------|
| Live Migration Overview | https://docs.openshift.com/container-platform/latest/virt/virtual_machines/virt-node-maintenance-virt.html |
| Configuring Live Migration Policies | https://docs.openshift.com/container-platform/latest/virt/virtual_machines/virt-node-maintenance-virt.html#virt-configuring-live-migration-policies_virt-node-maintenance-virt |
| Migration Methods (Pre-copy/Post-copy) | https://docs.openshift.com/container-platform/latest/virt/virtual_machines/virt-node-maintenance-virt.html#virt-about-live-migration_virt-node-maintenance-virt |

### KubeVirt

| Topic | Link |
|-------|------|
| KubeVirt Live Migration | https://kubevirt.io/user-guide/operations/live_migration/ |
| Migration Policies | https://kubevirt.io/user-guide/operations/live_migration/#migration-policies |
| Auto-Converge | https://kubevirt.io/user-guide/operations/live_migration/#auto-converge |
| Post-Copy | https://kubevirt.io/user-guide/operations/live_migration/#post-copy |

### Ansible

| Topic | Link |
|-------|------|
| kubernetes.core Collection | https://docs.ansible.com/ansible/latest/collections/kubernetes/core/ |
| k8s Module | https://docs.ansible.com/ansible/latest/collections/kubernetes/core/k8s_module.html |
| k8s_info Module | https://docs.ansible.com/ansible/latest/collections/kubernetes/core/k8s_info_module.html |
| k8s_drain Module | https://docs.ansible.com/ansible/latest/collections/kubernetes/core/k8s_drain_module.html |

---

## Common Troubleshooting Commands

```bash
# Check migration controller logs
oc logs -n openshift-cnv deployment/virt-controller | grep migration

# Check VMI events
oc get events -n <namespace> --field-selector involvedObject.name=<vmi-name>

# Check node resources
oc describe node <node-name> | grep -A 10 "Allocated resources"

# Check migration network (Multus)
oc get network-attachment-definitions -n openshift-cnv

# View migration bandwidth
oc get kubevirt -n openshift-cnv kubevirt-kubevirt-hyperconverged -o yaml | grep -A 5 migration
```

---

## Migration Phase Reference

| Phase | Description | Timeout Handling |
|-------|-------------|------------------|
| Pending | Migration request created | Check scheduler |
| Scheduling | Finding target node | Check node capacity |
| Scheduled | Target node selected | Check target readiness |
| PreparingTarget | Preparing target pod | Check virt-handler logs |
| TargetReady | Target is ready | Check memory transfer |
| Running | Memory copy in progress | Monitor dirty-rate |
| Succeeded | Migration complete | — |
| Failed | Migration failed | Check events, logs |
