---
type: Design
---

# Design Document: AAP + ACM Execution Path

## Overview

This document describes the enterprise multi-cluster approach for orchestrating OpenShift Virtualization VM migrations and node upgrades using Red Hat Advanced Cluster Management (ACM) integrated with Ansible Automation Platform (AAP). This path is suited for large-scale environments managing multiple OpenShift clusters with fleet-wide governance requirements.

---

## Architecture Narrative

### Component Interaction

```
+---------------------------+       +---------------------------+
|   Git Repository          |       |   ACM Hub                 |
|   (Policies, AppDefs)     |------>|   - Governance Framework  |
+---------------------------+       |   - PlacementRules        |
         ^                          |   - ApplicationSet       |
         | GitOps                   +---------------------------+
         |                                    |
+---------------------------+                 | Policy/Compliance
|   ArgoCD / OpenShift     |                 v
|   GitOps Operator        |       +---------------------------+
+---------------------------+       |   Managed Clusters        |
         |                          |   - Cluster A             |
         | Declarative State        |   - Cluster B             |
         v                          |   - Cluster C             |
+---------------------------+       +---------------------------+
|   OpenShift Clusters      |                 |
|   (Control Plane)         |                 | Policy Enforcement
+---------------------------+                 v
         |                          +---------------------------+
         | ACM-AAP Integration      |   AAP Controller          |
         |                          |   - Job Templates         |
         +------------------------->|   - Workflows             |
                                    |   - Automation Controller |
                                    +---------------------------+
                                              |
                                              | Ansible Playbooks
                                              v
                                    +---------------------------+
                                    |   OpenShift API           |
                                    |   (Per-cluster execution) |
                                    +---------------------------+
```

The AAP+ACM path introduces ACM as the fleet-wide governance layer. ACM policies define the desired state of clusters, and ACM can trigger AAP workflows when clusters enter specific compliance states (e.g., "maintenance required").

---

## Key Components

### 1. ACM Hub
The central control plane for multi-cluster management. The Hub hosts:
- **ManagedClusterSet** — Grouping of managed clusters.
- **PlacementRule / Placement** — Logic for selecting target clusters.
- **Governance Policies** — Configuration and compliance policies.
- **ApplicationSet** — GitOps-based application deployment.

### 2. Managed Clusters
OpenShift clusters enrolled in ACM. Each cluster runs:
- **Klusterlet** — Agent that communicates with Hub.
- **Application Controller** — Deploys workloads via GitOps.
- **Governance Policy Agent** — Enforces policies from Hub.

### 3. ACM Governance Framework
Policies define desired cluster state. Key policy types:

| Policy Type | Use Case |
|-------------|----------|
| `ConfigurationPolicy` | Enforce CR presence/absence (e.g., `MigrationPolicy`). |
| `CertificatePolicy` | Certificate expiration checks. |
| `IAMPolicy` | RBAC and role bindings. |
| `ComplianceType` | `musthave`, `mustnothave`, `shouldhave`. |

### 4. ACM-AAP Integration
ACM can delegate automation execution to AAP via:
- **AnsibleJob CR** — Custom resource that triggers an AAP Job Template.
- **Policy Automation** — Governance policy that triggers AnsibleJob on violation.

### 5. GitOps (ArgoCD / OpenShift GitOps)
Declarative deployment of:
- Cluster upgrade configurations.
- Node maintenance window definitions.
- VM placement policies.

---

## Upgrade Workflow Architecture

### Conceptual Flow

```
1. Policy Definition (Git)
   |
   v
2. ACM Hub Syncs Policy
   |
   v
3. Policy Distributed to Managed Clusters
   |
   v
4. Compliance Evaluation (Per Cluster)
   |
   +--(Compliant)--> No Action
   |
   +--(Non-Compliant: Maintenance Required)--> Trigger AAP Workflow
                                                     |
                                                     v
                                               AAP Executes Migration Playbook
                                                     |
                                                     v
                                               VMs Migrated, Node Upgraded
                                                     |
                                                     v
                                               Cluster Returns to Compliant State
```

### Policy-Driven Maintenance Trigger

ACM Governance Policies can include an `automation` section that triggers AAP on policy violation:

```yaml
apiVersion: policy.open-cluster-management.io/v1
kind: Policy
metadata:
  name: openshift-virt-node-maintenance
  namespace: policies
spec:
  remediationAction: enforce
  disabled: false
  policy-templates:
    - objectDefinition:
        apiVersion: policy.open-cluster-management.io/v1
        kind: ConfigurationPolicy
        metadata:
          name: node-maintenance-policy
        spec:
          remediationAction: inform
          object-templates:
            - complianceType: musthave
              objectDefinition:
                apiVersion: kubevirt.io/v1alpha1
                kind: MigrationPolicy
                metadata:
                  name: standard-vm-policy
                  namespace: openshift-cnv
                spec:
                  completionTimeoutPerGiB: 800
                  autoConverge: false
  automation:
    - name: aap-vm-migration-workflow
      type: Ansible
      extra_vars:
        target_node: "{{ .ManagedClusterLabels.node-for-maintenance }}"
        maintenance_reason: "{{ .PolicyViolationReason }}"
```

---

## PlacementRules for Cluster Targeting

ACM PlacementRules determine which clusters receive policies or applications.

### Example: Target Clusters with Virtualization Enabled

```yaml
apiVersion: cluster.open-cluster-management.io/v1beta1
kind: Placement
metadata:
  name: virt-enabled-clusters
  namespace: openshift-virtualization
spec:
  predicates:
    - requiredClusterSelector:
        labelSelector:
          matchLabels:
            openshift-virtualization: enabled
```

### Example: Target Specific Cluster for Maintenance

```yaml
apiVersion: cluster.open-cluster-management.io/v1beta1
kind: Placement
metadata:
  name: cluster-a-maintenance
  namespace: openshift-virtualization
spec:
  predicates:
    - requiredClusterSelector:
        labelSelector:
          matchLabels:
            cluster-name: cluster-a
            maintenance-window: scheduled
```

---

## AnsibleJob Custom Resource

The `AnsibleJob` CR bridges ACM and AAP.

### Create AnsibleJob

```yaml
apiVersion: tower.ansible.com/v1alpha1
kind: AnsibleJob
metadata:
  name: trigger-vm-migration
  namespace: openshift-virtualization
spec:
  tower_auth_secret: aap-credential-secret
  job_template_name: vm-migration-upgrade-workflow
  extra_vars:
    target_node: "worker-01"
    cluster_name: "cluster-a"
    maintenance_window_id: "CHG-12345"
```

**Key Fields:**
| Field | Description |
|-------|-------------|
| `tower_auth_secret` | Secret containing AAP credential (URL + token). |
| `job_template_name` | Name of AAP Job Template or Workflow. |
| `extra_vars` | Variables passed to AAP. |

### AnsibleJob Status

```yaml
status:
  status: successful
  job: 42
  url: https://aap-controller/api/v2/jobs/42/
```

---

## GitOps-Driven Upgrade Pattern

### ApplicationSet for Multi-Cluster Deployment

```yaml
apiVersion: argoproj.io/v1alpha1
kind: ApplicationSet
metadata:
  name: openshift-virt-upgrade
  namespace: openshift-gitops
spec:
  generators:
    - clusterDecisionResource:
        configMapRef: acm-placement-decision
        labelSelector:
          matchLabels:
            cluster.open-cluster-management.io/placement: virt-enabled-clusters
        requeueAfterSeconds: 180
  template:
    metadata:
      name: '{{name}}-virt-upgrade'
    spec:
      project: default
      source:
        repoURL: https://github.com/org/openshift-virt-config.git
        targetRevision: main
        path: overlays/{{name}}
      destination:
        server: '{{server}}'
        namespace: openshift-cnv
      syncPolicy:
        automated:
          prune: true
          selfHeal: true
```

---

## Fleet-Wide Upgrade Orchestration

### Step-by-Step Fleet Upgrade

1. **Define Upgrade Wave** — Label clusters with `upgrade-wave: 1`, `upgrade-wave: 2`, etc.
2. **Create PlacementRule per Wave** — Select clusters by wave label.
3. **Deploy Upgrade Policy** — Policy enforces cluster version or node state.
4. **Policy Violation Triggers AAP** — Non-compliant clusters trigger migration workflow.
5. **AAP Executes Migration** — VMs migrated, node upgraded.
6. **Compliance Restored** — Cluster returns to compliant state.
7. **Proceed to Next Wave** — Repeat for subsequent waves.

---

## RBAC for ACM-AAP Integration

### ACM Hub RBAC

| Role | Permissions |
|------|-------------|
| `cluster-admin` | Full access to all ACM resources. |
| `cluster-manager` | Manage ManagedClusters, Placements. |
| `policy-admin` | Create/update Governance Policies. |
| `application-admin` | Manage ApplicationSets, Subscriptions. |

### AAP RBAC (for ACM Integration)

| Role | Permissions |
|------|-------------|
| `acm-integration` | Launch specific workflows triggered by ACM. |
| `workflow-executor` | Launch workflows with extra_vars. |

### ServiceAccount for ACM-AAP Auth

```yaml
apiVersion: v1
kind: Secret
metadata:
  name: aap-credential-secret
  namespace: openshift-virtualization
type: Opaque
stringData:
  host: https://aap-controller.example.com
  token: <aap-api-token>
```

---

## Failure Modes & Recovery

| Failure Point | Detection | Recovery Action |
|---------------|-----------|-----------------|
| Policy sync failure | ACM Hub shows `Policy: NonCompliant` | Check Git connectivity, PlacementRule. |
| AnsibleJob not created | No AnsibleJob CR in namespace | Check policy automation config, RBAC. |
| AAP authentication failure | AnsibleJob status `failed` | Check secret, token rotation. |
| Migration timeout | AAP job fails | Workflow rollback, manual intervention. |
| Cluster disconnected | ACM shows `ManagedCluster: Unknown` | Check network, klusterlet health. |

---

## Pre-Conditions

| Requirement | Description |
|-------------|-------------|
| ACM Hub | Installed and configured (assume pre-existing). |
| Managed Clusters | Enrolled in ACM Hub. |
| AAP Controller | Installed and licensed (assume pre-existing). |
| AAP Credential Secret | Secret in each managed cluster namespace. |
| GitOps Operator | OpenShift GitOps (ArgoCD) installed on Hub and managed clusters. |
| AnsibleJob CRD | Installed via AAP Operator. |
| Network Connectivity | Hub can reach managed clusters; managed clusters can reach AAP. |

---

## Rollback Strategy

### ACM-Level Rollback

1. **Remove Policy** — Delete or disable the upgrade policy.
2. **Restore Previous Configuration** — GitOps reverts cluster to previous state.
3. **Manual Node Recovery** — Uncordon nodes, reschedule VMs.

### AAP-Level Rollback

1. **Trigger Rollback Workflow** — AAP job template `node-rollback`.
2. **Verify Rollback** — Check cluster compliance status.

---

## Next Steps

After reviewing this design, proceed to:
- **checklist.md** — Step-by-step guide for configuring ACM-AAP integration.
- **reference.md** — ACM CRD schemas, AnsibleJob reference, external documentation.
