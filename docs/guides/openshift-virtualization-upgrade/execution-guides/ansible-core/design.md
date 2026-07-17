---
type: Design
---

# Design Document: Ansible Core Execution Path

## Overview

This document describes the runbook-driven approach for orchestrating OpenShift Virtualization VM migrations and node upgrades using Ansible Core (Engine-only). This path is suited for environments without centralized automation platforms or where a simple, scriptable workflow is preferred.

---

## Architecture Narrative

### Component Interaction

```
+-------------------+       +---------------------------+
|  Ansible Control  |       |   OpenShift API Server    |
|  Node (Jumphost)  |------>|   (kubeconfig auth)       |
+-------------------+       +---------------------------+
         |                              |
         | kubernetes.core              | KubeVirt CRDs
         v                              v
+-------------------+       +---------------------------+
|  Ansible Playbook |------>| VirtualMachineInstance    |
|  (local execution)|       | Migration CRs, Nodes      |
+-------------------+       +---------------------------+
```

The Ansible Core path operates as a single-threaded, CLI-driven workflow. The operator executes playbooks from a control node (typically a jumphost or administrator workstation) that has network access to the OpenShift API and a valid `kubeconfig` for authentication.

### Execution Flow

1. **Pre-Migration Assessment** — Playbook queries cluster for VMs on target node, checks migration eligibility, validates destination node capacity.
2. **Migration Policy Application** — Playbook applies tuned `MigrationPolicy` CR based on VM class (memory size, dirty-rate profile).
3. **Live Migration Execution** — Playbook creates `VirtualMachineInstanceMigration` CRs and polls for completion.
4. **Node Drain** — Playbook cordons and drains the target node once all VMs have migrated.
5. **Node Upgrade** — Operator executes OpenShift upgrade steps (or playbook triggers cluster upgrade via API).
6. **Node Validation & Return** — Playbook verifies node health, uncordons, and returns node to service.
7. **Rollback Gates** — At each phase, the playbook checks for failure conditions and can halt execution.

---

## Key Design Decisions

### 1. Direct API Interaction
Ansible interacts directly with the OpenShift API using the `kubernetes.core` collection. No intermediate operator or controller is required. This keeps the design simple but places responsibility on the playbook for state management and error handling.

### 2. MigrationPolicy Over Per-VM Tuning
Rather than tuning migration parameters on each VM individually, the design uses cluster-scoped `MigrationPolicy` CRDs that match VMs by label selector. This enables consistent policy application by workload class.

### 3. Sequential Execution
The playbook processes one node at a time. Parallel execution is possible but increases complexity and risk; sequential processing is recommended for initial adoption.

### 4. Manual Upgrade Trigger
The node upgrade step (OpenShift cluster upgrade or OS patching) may be manual or delegated to cluster-version operators. The playbook focuses on pre-migration and drain orchestration.

---

## Pre-Conditions

| Requirement | Description |
|-------------|-------------|
| Ansible Core | Version 2.14+ installed on control node |
| `kubernetes.core` Collection | Installed via `ansible-galaxy` |
| `python-openshift` | Python library for OpenShift API |
| `kubeconfig` | Valid cluster-admin credentials |
| Network Access | Control node can reach OpenShift API (port 6443) |
| VM Storage | Shared storage (e.g., PVCs) accessible across nodes |
| Migration Network | (Optional) Dedicated network configured for migration traffic |

---

## Failure Modes & Recovery

| Failure Point | Detection | Recovery Action |
|---------------|-----------|-----------------|
| Migration timeout | Playbook polls VMI migration status | Halt playbook; investigate VM; manual migration or rollback |
| Migration stuck | Progress timeout exceeded | Cancel migration CR; analyze dirty-rate; consider auto-converge |
| Drain failure | Pod disruption budget violations | Abort drain; restore workloads manually; adjust PDBs |
| Node upgrade failure | Node not Ready after reboot | Isolate node; escalate to cluster admin |

---

## Rollback Strategy

The Ansible Core path does not have an automated rollback. If a phase fails:
1. **Migration failure** — VM remains on source node; no action needed.
2. **Drain failure** — Uncordon node; VMs may need manual rescheduling.
3. **Upgrade failure** — Follow standard OpenShift node recovery procedures.

All rollback actions are manual and documented in the checklist.

---

## Limitations

- No centralized audit logging (logs live on control node).
- Credentials stored locally on control node (no vault integration).
- No RBAC enforcement on who can execute upgrades.
- Single-threaded execution limits throughput for large clusters.
- No multi-cluster coordination.

---

## Next Steps

After reviewing this design, proceed to:
- **checklist.md** — Step-by-step execution guide.
- **reference.md** — CRD schemas, module references, and external documentation.
