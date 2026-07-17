---
type: Design
---

# Design Document: Ansible Automation Platform (AAP) Execution Path

## Overview

This document describes the centralized, auditable, and GitOps-adjacent approach for orchestrating OpenShift Virtualization VM migrations and node upgrades using Red Hat Ansible Automation Platform (AAP). This path is suited for enterprise environments requiring RBAC, credential management, audit trails, and workflow visualization.

---

## Architecture Narrative

### Component Interaction

```
+---------------------------+       +---------------------------+
|   Git Repository          |       |   AAP Controller          |
|   (Playbooks, Inventories)|------>|   - Job Templates         |
+---------------------------+       |   - Workflows             |
         ^                          |   - Credentials           |
         | Webhook/API              |   - Projects              |
         |                          +---------------------------+
+---------------------------+                 |
|   Trigger Sources         |                 | Execution
|   - Git Push              |                 v
|   - REST API Call         |       +---------------------------+
|   - Schedule (Cron)       |------>|   Execution Environment   |
+---------------------------+       |   (EE Container)          |
                                    +---------------------------+
                                              |
                                              | kubernetes.core
                                              v
                                    +---------------------------+
                                    |   OpenShift API Server    |
                                    |   (ServiceAccount auth)   |
                                    +---------------------------+
                                              |
                                              v
                                    +---------------------------+
                                    |   KubeVirt CRDs           |
                                    |   - VirtualMachineInstance|
                                    |   - MigrationPolicy       |
                                    |   - Nodes                 |
                                    +---------------------------+
```

The AAP path introduces a centralized control plane between the automation code (in Git) and the target OpenShift clusters. AAP Controller manages execution environments, credentials, RBAC, and audit logging.

---

## Key Components

### 1. Projects
A Project in AAP is a logical collection of Ansible content (playbooks, roles, inventories) sourced from a Git repository. Projects are synced periodically or on-demand.

**Design Pattern:**
- One Project per environment (e.g., `openshift-virt-prod`, `openshift-virt-test`).
- Project syncs from a Git branch matching the environment.
- Playbooks are version-controlled; changes go through Git PRs.

### 2. Job Templates
A Job Template defines the parameters for running a single playbook: which Project, Inventory, Credential, and Execution Environment to use.

**Design Pattern:**
- Granular templates for each operation (e.g., `vm-pre-migrate`, `node-drain`, `node-validate`).
- Templates accept `extra_vars` for runtime parameters (node name, VM class).
- Survey (form) inputs allow operators to provide values via UI.

### 3. Workflow Job Templates
A Workflow chains multiple Job Templates into a directed acyclic graph (DAG). Workflows enable complex orchestration with conditional logic, parallelism, and convergence.

**Design Pattern:**
```
[Start] --> [Pre-Migration Assessment]
              |
              v
        [Migration Policy Apply]
              |
              v
        [VM Live Migration]
              |
              +--(success)--> [Node Drain]
              |                     |
              |                     v
              |               [Node Upgrade]
              |                     |
              |                     v
              |               [Node Validate]
              |                     |
              +---------------------+
                                    v
                              [End - Success]
              
              +--(failure)--> [Alert / Rollback]
```

### 4. Credentials
AAP securely stores secrets (kubeconfig tokens, API keys, vault passwords) in encrypted form.

**Design Pattern:**
- `OpenShift` or `Kubernetes` credential type for cluster access.
- Credential bound to specific Job Templates via RBAC.
- ServiceAccount tokens auto-rotated by OpenShift.

### 5. Execution Environments (EE)
Container images that bundle Ansible Core, collections, and Python dependencies.

**Design Pattern:**
- Custom EE includes `kubernetes.core`, `python-openshift`, `kubevirt` client tools.
- EE versioned alongside playbook versions for reproducibility.

### 6. Inventories
Define the target clusters and their connection parameters.

**Design Pattern:**
- Dynamic inventory sourced from AAP or external CMDB.
- Groups: `cluster_prod`, `cluster_test`, `cluster_dev`.
- Host variables: `kubeconfig_path`, `cluster_api_url`.

---

## GitOps-ish Workflow Patterns

### Pattern A: Git Push Triggered

```
Developer pushes to Git --> Webhook --> AAP Project Sync --> Workflow Trigger
```

1. Developer merges PR to `main` branch.
2. Git platform (GitHub, GitLab) sends webhook to AAP Controller.
3. AAP syncs Project and optionally triggers a Workflow Job.
4. Workflow executes upgrade sequence.

**Use Case:** Controlled promotion from test to prod. Merge triggers prod workflow.

### Pattern B: REST API Triggered

```
External System --> REST API --> AAP Workflow Job
```

1. External scheduler, monitoring system, or chatbot calls AAP REST API.
2. API call includes `extra_vars` (node name, cluster, window).
3. AAP launches Workflow Job with provided parameters.

**Use Case:** Integration with ITSM, change management, or on-call rotation.

### Pattern C: Scheduled (Cron)

```
AAP Schedule --> Workflow Job
```

1. AAP Schedule defined with cron expression.
2. Schedule launches Workflow Job at designated times.
3. Workflow executes during maintenance windows.

**Use Case:** Regular patching cycles, capacity-driven maintenance.

---

## Workflow Visualizer Pattern

The AAP Workflow Visualizer allows designers to construct DAGs graphically. For VM migration upgrades, the recommended workflow:

```
                                    +------------------+
                                    |     START        |
                                    +------------------+
                                             |
                                             v
                                    +------------------+
                                    | Pre-Migration    |
                                    | Assessment       |
                                    +------------------+
                                             |
                                             v
                                    +------------------+
                                    | Apply Migration  |
                                    | Policy by Class  |
                                    +------------------+
                                             |
                                             v
                                    +------------------+
                                    | Live Migrate VMs |
                                    +------------------+
                                     /              \
                            (success)                (failure)
                               |                         |
                               v                         v
                      +------------------+      +------------------+
                      | Drain Node       |      | Send Alert       |
                      +------------------+      +------------------+
                               |                         |
                               v                         v
                      +------------------+      +------------------+
                      | Upgrade Node     |      | Rollback Gate    |
                      +------------------+      +------------------+
                               |
                               v
                      +------------------+
                      | Validate Node    |
                      +------------------+
                               |
                               v
                      +------------------+
                      |      END         |
                      +------------------+
```

**Workflow Nodes:**
- **On Success:** Continue to next step.
- **On Failure:** Branch to alerting and rollback evaluation.
- **Always:** Run cleanup tasks (e.g., un-cordon if workflow aborts).

---

## RBAC Design

| Role | Permissions |
|------|-------------|
| `Workflow Executor` | Launch specific workflow templates; provide survey inputs. |
| `Job Template Admin` | Create/edit job templates; modify surveys. |
| `Credential Admin` | Manage credentials; rotate tokens. |
| `Auditor` | View job logs, workflow runs, audit history. |
| `Project Admin` | Manage Git project sync; update inventory. |

**Team Structure:**
- `openshift-ops` team: Workflow Executor + Auditor.
- `openshift-admin` team: Job Template Admin + Credential Admin.
- `platform-security` team: Auditor only.

---

## Audit & Logging

AAP provides centralized audit logging:

| Log Type | Content |
|----------|---------|
| Job Log | STDOUT/STDERR from playbook execution. |
| Workflow Log | DAG traversal, node status, timing. |
| Audit Log | Who launched what, when, with which parameters. |
| Credential Log | When credentials were used (not the values). |

**Retention:**
- Job logs: 30 days (configurable).
- Audit logs: 1 year (compliance requirement).

**Integration:**
- Forward logs to Splunk, ELK, or SIEM via AAP logging configuration.

---

## Failure Modes & Recovery

| Failure Point | Detection | Recovery Action |
|---------------|-----------|-----------------|
| Project sync failure | AAP UI shows "Failed to sync" | Check Git connectivity, SSH keys. |
| Credential expired | Job fails with auth error | Rotate token, update credential in AAP. |
| Migration timeout | Playbook task timeout | Workflow branches to alert; manual intervention. |
| Node drain failure | Job fails mid-workflow | Rollback node, uncordon, escalate. |
| EE missing dependencies | Playbook fails on import | Rebuild EE, redeploy. |

---

## Pre-Conditions

| Requirement | Description |
|-------------|-------------|
| AAP Controller | Installed and licensed (assume pre-existing). |
| Execution Environment | Custom EE with `kubernetes.core`, `openshift` Python libs. |
| Git Repository | Playbooks, roles, inventories in version control. |
| OpenShift Credentials | ServiceAccount tokens stored in AAP. |
| Network Connectivity | AAP can reach OpenShift API (port 6443). |
| RBAC Configured | Teams and roles defined in AAP. |

---

## Rollback Strategy

AAP workflows support explicit rollback nodes:

1. **On Failure** branch triggers a `Rollback` Job Template.
2. `Rollback` playbook uncordons node, cancels pending migrations.
3. Workflow ends with `Rollback Complete` status.
4. Operator reviews logs and decides on retry or escalation.

---

## Next Steps

After reviewing this design, proceed to:
- **checklist.md** — Step-by-step guide for setting up and executing AAP workflows.
- **reference.md** — Job Template schemas, REST API references, external documentation.
