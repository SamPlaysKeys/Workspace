---
type: Guide
---

# Execution Checklist: AAP + ACM Path

This checklist provides a step-by-step guide for configuring ACM-AAP integration to orchestrate OpenShift Virtualization VM migrations and node upgrades across a multi-cluster fleet. Complete each phase in order.

---

## Phase 0: Pre-Execution Setup

### 0.1 Verify ACM Hub Topology

| Step | Action | Verification |
|------|--------|--------------|
| 0.1.1 | Verify ACM Hub is installed: `oc get csv -n open-cluster-management` | CSV shows `advanced-cluster-management` |
| 0.1.2 | Verify managed clusters: `oc get managedclusters` | Clusters listed with `status: True` |
| 0.1.3 | Verify klusterlet on each managed cluster: `oc get klusterlet -n open-cluster-management-agent` | Klusterlet running |
| 0.1.4 | Label clusters with Virtualization enabled: `oc label managedcluster <name> openshift-virtualization=enabled` | Labels applied |

### 0.2 Verify AAP Controller

| Step | Action | Verification |
|------|--------|--------------|
| 0.2.1 | Verify AAP Controller is accessible: `curl -k https://<aap-controller>/api/v2/` | API responds |
| 0.2.2 | Create AAP API token for ACM integration | Token created |
| 0.2.3 | Verify Workflow Job Template exists: `vm-migration-upgrade-workflow` | Template exists in AAP |

### 0.3 Install AnsibleJob CRD

| Step | Action | Verification |
|------|--------|--------------|
| 0.3.1 | Install AAP Operator on managed clusters (if not present): `oc apply -f aap-operator-subscription.yaml` | Operator installed |
| 0.3.2 | Verify AnsibleJob CRD: `oc get crd ansiblejobs.tower.ansible.com` | CRD exists |
| 0.3.3 | Verify Resource Ansible Operator is running | Pod running in namespace |

---

## Phase 1: Create ACM-AAP Authentication Secret

| Step | Action | Verification |
|------|--------|--------------|
| 1.1 | Create namespace for ACM resources: `oc create namespace openshift-virtualization-policies` | Namespace created |
| 1.2 | Create Secret with AAP credentials: | Secret created |

**Secret Definition:**
```yaml
apiVersion: v1
kind: Secret
metadata:
  name: aap-credential-secret
  namespace: openshift-virtualization-policies
type: Opaque
stringData:
  host: https://aap-controller.example.com
  token: <aap-api-token>
```

| Step | Action | Verification |
|------|--------|--------------|
| 1.3 | Apply Secret: `oc apply -f aap-credential-secret.yaml` | Secret applied |
| 1.4 | Verify Secret: `oc get secret aap-credential-secret -n openshift-virtualization-policies` | Secret exists |

---

## Phase 2: Create PlacementRules

### 2.1 Placement for All Virtualization Clusters

| Step | Action | Verification |
|------|--------|--------------|
| 2.1.1 | Create Placement for Virtualization-enabled clusters | Placement created |

**Placement Definition:**
```yaml
apiVersion: cluster.open-cluster-management.io/v1beta1
kind: Placement
metadata:
  name: virt-enabled-clusters
  namespace: openshift-virtualization-policies
spec:
  predicates:
    - requiredClusterSelector:
        labelSelector:
          matchLabels:
            openshift-virtualization: enabled
```

### 2.2 Placement for Maintenance Target Cluster

| Step | Action | Verification |
|------|--------|--------------|
| 2.2.1 | Create Placement for specific cluster under maintenance | Placement created |

**Placement Definition:**
```yaml
apiVersion: cluster.open-cluster-management.io/v1beta1
kind: Placement
metadata:
  name: cluster-a-maintenance
  namespace: openshift-virtualization-policies
spec:
  numberOfClusters: 1
  predicates:
    - requiredClusterSelector:
        labelSelector:
          matchLabels:
            cluster-name: cluster-a
            maintenance-window: scheduled
```

| Step | Action | Verification |
|------|--------|--------------|
| 2.2.2 | Apply Placements: `oc apply -f placements.yaml` | Placements applied |
| 2.2.3 | Verify Placement Decisions: `oc get placementdecisions -n openshift-virtualization-policies` | Decisions show cluster names |

---

## Phase 3: Create Governance Policy

### 3.1 ConfigurationPolicy for MigrationPolicy

| Step | Action | Verification |
|------|--------|--------------|
| 3.1.1 | Create ConfigurationPolicy to enforce MigrationPolicy | Policy created |

**Policy Definition:**
```yaml
apiVersion: policy.open-cluster-management.io/v1
kind: Policy
metadata:
  name: openshift-virt-migration-policy
  namespace: openshift-virtualization-policies
  annotations:
    policy.open-cluster-management.io/standards: NIST
    policy.open-cluster-management.io/categories: CM Configuration Management
    policy.open-cluster-management.io/controls: CM-2 Baseline Configuration
spec:
  remediationAction: enforce
  disabled: false
  policy-templates:
    - objectDefinition:
        apiVersion: policy.open-cluster-management.io/v1
        kind: ConfigurationPolicy
        metadata:
          name: migration-policy-config
        spec:
          remediationAction: enforce
          severity: medium
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
                  parallelMigrationsPerCluster: 5
                  parallelOutboundMigrationsPerNode: 2
                  autoConverge: false
                  allowPostCopy: false
```

### 3.2 Policy with Automation (Triggers AAP)

| Step | Action | Verification |
|------|--------|--------------|
| 3.2.1 | Create Policy with automation section | Policy created |

**Policy with Automation:**
```yaml
apiVersion: policy.open-cluster-management.io/v1
kind: Policy
metadata:
  name: openshift-virt-node-maintenance
  namespace: openshift-virtualization-policies
spec:
  remediationAction: inform
  disabled: false
  policy-templates:
    - objectDefinition:
        apiVersion: policy.open-cluster-management.io/v1
        kind: ConfigurationPolicy
        metadata:
          name: node-maintenance-check
        spec:
          remediationAction: inform
          severity: high
          object-templates:
            - complianceType: musthave
              objectDefinition:
                apiVersion: v1
                kind: Node
                metadata:
                  name: worker-01
                  labels:
                    maintenance-required: "false"
  automation:
    - name: aap-vm-migration-workflow
      type: Ansible
      extra_vars:
        target_node: "{{ .Object.metadata.name }}"
        maintenance_reason: "node-maintenance-required"
```

| Step | Action | Verification |
|------|--------|--------------|
| 3.2.2 | Apply Policies: `oc apply -f policies.yaml` | Policies applied |
| 3.2.3 | Verify Policy Status: `oc get policy -n openshift-virtualization-policies` | Policies listed |

---

## Phase 4: Create PlacementBinding

| Step | Action | Verification |
|------|--------|--------------|
| 4.1 | Create PlacementBinding to bind Policy to Placement | Binding created |

**PlacementBinding Definition:**
```yaml
apiVersion: policy.open-cluster-management.io/v1
kind: PlacementBinding
metadata:
  name: virt-clusters-policy-binding
  namespace: openshift-virtualization-policies
placementRef:
  name: virt-enabled-clusters
  kind: Placement
  apiGroup: cluster.open-cluster-management.io
subjects:
  - name: openshift-virt-migration-policy
    kind: Policy
    apiGroup: policy.open-cluster-management.io
  - name: openshift-virt-node-maintenance
    kind: Policy
    apiGroup: policy.open-cluster-management.io
```

| Step | Action | Verification |
|------|--------|--------------|
| 4.2 | Apply PlacementBinding: `oc apply -f placementbinding.yaml` | Binding applied |
| 4.3 | Verify Policies distributed: `oc get configurationpolicy -A` | Policies exist on managed clusters |

---

## Phase 5: Test AnsibleJob Creation

| Step | Action | Verification |
|------|--------|--------------|
| 5.1 | Manually create AnsibleJob to test integration | AnsibleJob created |

**AnsibleJob Test:**
```yaml
apiVersion: tower.ansible.com/v1alpha1
kind: AnsibleJob
metadata:
  name: test-vm-migration
  namespace: openshift-virtualization-policies
spec:
  tower_auth_secret: aap-credential-secret
  job_template_name: vm-migration-upgrade-workflow
  extra_vars:
    target_node: "worker-01"
    cluster_name: "cluster-a"
    maintenance_window_id: "TEST-001"
```

| Step | Action | Verification |
|------|--------|--------------|
| 5.2 | Apply AnsibleJob: `oc apply -f test-ansiblejob.yaml` | AnsibleJob created |
| 5.3 | Check AnsibleJob status: `oc get ansiblejob test-vm-migration -o yaml` | Status shows `successful` or `running` |
| 5.4 | Verify AAP job launched: Check AAP Controller UI | Job visible in AAP |
| 5.5 | Check AAP job log for execution details | Log shows successful run |

---

## Phase 6: Trigger Maintenance via Policy Violation

### 6.1 Simulate Policy Violation

| Step | Action | Verification |
|------|--------|--------------|
| 6.1.1 | Label target cluster for maintenance: `oc label managedcluster cluster-a maintenance-window=scheduled` | Label applied |
| 6.1.2 | Verify Placement Decision updates: `oc get placementdecision -n openshift-virtualization-policies -o yaml` | Cluster-a in decisions |
| 6.1.3 | Trigger policy violation: Modify Node label on cluster-a | Node labeled |
| 6.1.4 | Check Policy compliance: `oc get policy -n openshift-virtualization-policies` | Policy shows `NonCompliant` |
| 6.1.5 | Verify AnsibleJob created: `oc get ansiblejob -n openshift-virtualization-policies` | AnsibleJob created automatically |
| 6.1.6 | Monitor AAP job execution | Workflow runs to completion |

### 6.2 Verify End-to-End Flow

| Step | Action | Verification |
|------|--------|--------------|
| 6.2.1 | Verify VMs migrated on target cluster | VMs running on other nodes |
| 6.2.2 | Verify node drained and upgraded | Node upgraded |
| 6.2.3 | Verify Policy returns to Compliant | Policy shows `Compliant` |
| 6.2.4 | Verify AnsibleJob status is `successful` | AnsibleJob succeeded |

---

## Phase 7: Fleet-Wide Upgrade Waves

### 7.1 Define Upgrade Waves

| Step | Action | Verification |
|------|--------|--------------|
| 7.1.1 | Label clusters by wave: `oc label managedcluster cluster-a upgrade-wave=1` | Labels applied |
| 7.1.2 | Create Placement per wave | Placements created |

**Wave 1 Placement:**
```yaml
apiVersion: cluster.open-cluster-management.io/v1beta1
kind: Placement
metadata:
  name: upgrade-wave-1
  namespace: openshift-virtualization-policies
spec:
  predicates:
    - requiredClusterSelector:
        labelSelector:
          matchLabels:
            upgrade-wave: "1"
```

### 7.2 Execute Wave-by-Wave

| Step | Action | Verification |
|------|--------|--------------|
| 7.2.1 | Apply Wave 1 PlacementBinding | Wave 1 clusters receive policy |
| 7.2.2 | Monitor Wave 1 completion | All Wave 1 clusters upgraded |
| 7.2.3 | Apply Wave 2 PlacementBinding | Wave 2 clusters receive policy |
| 7.2.4 | Continue until all waves complete | All clusters upgraded |

---

## Phase 8: GitOps Integration (Optional)

### 8.1 Create ApplicationSet for Policy Deployment

| Step | Action | Verification |
|------|--------|--------------|
| 8.1.1 | Create ApplicationSet referencing policies in Git | ApplicationSet created |

**ApplicationSet Definition:**
```yaml
apiVersion: argoproj.io/v1alpha1
kind: ApplicationSet
metadata:
  name: openshift-virt-policies
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
      name: '{{name}}-virt-policies'
    spec:
      project: default
      source:
        repoURL: https://github.com/org/openshift-virt-config.git
        targetRevision: main
        path: policies/base
      destination:
        server: '{{server}}'
        namespace: openshift-cnv
      syncPolicy:
        automated:
          prune: true
          selfHeal: true
```

| Step | Action | Verification |
|------|--------|--------------|
| 8.1.2 | Apply ApplicationSet: `oc apply -f applicationset.yaml` | ApplicationSet created |
| 8.1.3 | Verify Applications created per cluster: `oc get applications -n openshift-gitops` | Applications visible |
| 8.1.4 | Verify policies synced to managed clusters | Policies applied via GitOps |

---

## Phase 9: Monitoring & Observability

| Step | Action | Verification |
|------|--------|--------------|
| 9.1 | Configure ACM Policy Controller to send alerts | Alerts configured |
| 9.2 | Forward AAP job logs to centralized logging | Logs forwarding |
| 9.3 | Create Grafana dashboard for migration metrics | Dashboard created |
| 9.4 | Set up alerting for failed migrations | Alerts configured |

---

## Troubleshooting Guide

| Issue | Detection | Resolution |
|-------|-----------|------------|
| Placement Decision empty | `oc get placementdecision` shows no clusters | Check cluster labels, Placement selector |
| Policy not distributed | `oc get configurationpolicy -A` missing | Check PlacementBinding, Policy namespace |
| AnsibleJob not created | No AnsibleJob CR on violation | Check policy `automation` section, RBAC |
| AnsibleJob auth failure | AnsibleJob status `failed`, 401 error | Check Secret, token validity |
| AAP job not found | AnsibleJob fails with template not found | Verify job_template_name matches AAP |
| Policy stuck NonCompliant | Policy status not changing | Check CR validity, cluster connectivity |

---

## Quick Reference: ACM Commands

```bash
# List managed clusters
oc get managedclusters

# Label a cluster
oc label managedcluster <name> <key>=<value>

# List placements
oc get placements -n openshift-virtualization-policies

# List placement decisions
oc get placementdecisions -n openshift-virtualization-policies -o yaml

# List policies
oc get policy -n openshift-virtualization-policies

# Check policy compliance
oc get configurationpolicy -A

# Create AnsibleJob
oc apply -f ansiblejob.yaml

# Check AnsibleJob status
oc get ansiblejob <name> -o yaml
```
