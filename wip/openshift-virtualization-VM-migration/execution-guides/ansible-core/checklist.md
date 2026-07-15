---
type: Guide
---

# Execution Checklist: Ansible Core Path

This checklist provides a step-by-step guide for executing VM migrations and node upgrades using Ansible Core. Complete each phase in order; do not skip verification gates.

---

## Phase 0: Pre-Execution Setup

| Step | Action | Verification |
|------|--------|--------------|
| 0.1 | Install `kubernetes.core` collection: `ansible-galaxy collection install kubernetes.core` | Collection installed |
| 0.2 | Install `python-openshift`: `pip install openshift` | Library available |
| 0.3 | Verify `kubeconfig` is valid and has cluster-admin rights: `oc whoami` | Returns username with admin role |
| 0.4 | Verify Ansible can reach OpenShift API: `ansible localhost -m kubernetes.core.k8s_info -a "api_version=v1 kind=Namespace"` | Returns namespace list |
| 0.5 | Export `KUBECONFIG` environment variable or embed path in playbook | Environment set |

---

## Phase 1: Pre-Migration Assessment

| Step | Action | Verification |
|------|--------|--------------|
| 1.1 | Identify target node for upgrade | Node hostname recorded |
| 1.2 | List VMs running on target node: `oc get vmi -o wide` | VM list captured |
| 1.3 | Check for VMs with `spec.running: false` (not migratable) | All target VMs are running |
| 1.4 | Verify destination nodes have sufficient capacity (CPU, memory, storage) | Capacity confirmed |
| 1.5 | Check for `PodDisruptionBudget` constraints: `oc get pdb -A` | PDBs allow migration |
| 1.6 | Identify VM workload classes (memory size, dirty-rate profile) | Classes documented |

---

## Phase 2: Migration Policy Application

| Step | Action | Verification |
|------|--------|--------------|
| 2.1 | Review existing `MigrationPolicy` CRs: `oc get migrationpolicy -n openshift-cnv` | Policies listed |
| 2.2 | Apply or update `MigrationPolicy` matching VM class labels | Policy created/updated |
| 2.3 | Verify policy is attached to target VMs: `oc get vmi -o yaml \| grep migrationPolicyName` | Policy name shown |
| 2.4 | If using dedicated migration network, verify `MigrationPolicy` references correct network | Network attachment defined |

---

## Phase 3: Live Migration Execution

| Step | Action | Verification |
|------|--------|--------------|
| 3.1 | Create `VirtualMachineInstanceMigration` CR for each VM | Migration CR created |
| 3.2 | Monitor migration status: `oc get vmim <migration-name> -o yaml` | Status is `Succeeded` |
| 3.3 | If migration times out, check VM dirty-rate: `virtctl migrate <vm-name> --dry-run` | Dirty-rate analyzed |
| 3.4 | For stuck migrations, consider cancelling: `oc delete vmim <migration-name>` | Migration cancelled |
| 3.5 | Re-attempt with adjusted policy (auto-converge, post-copy) or manual intervention | Migration succeeded |
| 3.6 | Verify all VMs are running on other nodes: `oc get vmi -o wide` | No VMs on target node |

---

## Phase 4: Node Drain

| Step | Action | Verification |
|------|--------|--------------|
| 4.1 | Cordon target node: `oc adm cordon <node-name>` | Node is `SchedulingDisabled` |
| 4.2 | Drain node: `oc adm drain <node-name> --ignore-daemonsets --delete-emptydir-data --force` | Drain completed |
| 4.3 | Verify no pods remain (except DaemonSets): `oc get pods --all-namespaces --field-selector spec.nodeName=<node-name>` | Only DaemonSets remain |
| 4.4 | Verify node is `NotReady`: `oc get nodes` | Node status is `NotReady,SchedulingDisabled` |

---

## Phase 5: Node Upgrade

| Step | Action | Verification |
|------|--------|--------------|
| 5.1 | Trigger OpenShift cluster upgrade (if applicable): `oc adm upgrade --to=<version>` | Upgrade started |
| 5.2 | OR patch node OS (if manual): SSH to node, run package updates | OS patched |
| 5.3 | Reboot node if required | Node rebooted |
| 5.4 | Wait for node to become `Ready`: `oc get nodes` | Node is `Ready` |

---

## Phase 6: Node Validation & Return

| Step | Action | Verification |
|------|--------|--------------|
| 6.1 | Verify node is `Ready`: `oc get nodes` | Status is `Ready` |
| 6.2 | Verify system pods on node: `oc get pods -n openshift-* --field-selector spec.nodeName=<node-name>` | Pods running |
| 6.3 | Uncordon node: `oc adm uncordon <node-name>` | Node schedulable |
| 6.4 | Verify VMs can be scheduled back (optional): `oc get vmi -o wide` | VMs distributed as expected |

---

## Phase 7: Rollback (If Failure)

| Step | Action | Verification |
|------|--------|--------------|
| 7.1 | If migration failed, VM remains on source — no action needed | VM still running |
| 7.2 | If drain failed, uncordon node: `oc adm uncordon <node-name>` | Node schedulable |
| 7.3 | If upgrade failed, isolate node and escalate | Node cordoned, escalated |
| 7.4 | Document failure in runbook log | Failure documented |

---

## Phase 8: Record Keeping

| Step | Action | Verification |
|------|--------|--------------|
| 8.1 | Save playbook execution log | Log archived |
| 8.2 | Document migration durations per VM class | Metrics recorded |
| 8.3 | Note any policy adjustments made | Adjustments documented |
| 8.4 | Proceed to next node (repeat from Phase 1) | Next node identified |

---

## Quick Reference Commands

```bash
# List VMs on a node
oc get vmi -o wide | grep <node-name>

# Create migration CR
cat <<EOF | oc apply -f -
apiVersion: kubevirt.io/v1
kind: VirtualMachineInstanceMigration
metadata:
  name: <migration-name>
spec:
  vmiName: <vmi-name>
EOF

# Monitor migration
oc get vmim <migration-name> -o yaml

# Drain node
oc adm drain <node-name> --ignore-daemonsets --delete-emptydir-data --force

# Cordon/Uncordon
oc adm cordon <node-name>
oc adm uncordon <node-name>
```
