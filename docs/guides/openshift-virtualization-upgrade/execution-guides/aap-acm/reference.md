---
type: Reference
---

# Reference Document: AAP + ACM Path

This document provides ACM CRD schemas, AnsibleJob reference, Placement/PlacementBinding schemas, and links to external documentation for the AAP+ACM execution path.

---

## ACM Core CRDs

### ManagedCluster (cluster.open-cluster-management.io/v1)

Represents a cluster enrolled in ACM.

```yaml
apiVersion: cluster.open-cluster-management.io/v1
kind: ManagedCluster
metadata:
  name: cluster-a
  labels:
    openshift-virtualization: enabled
    upgrade-wave: "1"
    cluster-name: cluster-a
spec:
  hubAcceptsClient: true
  leaseDurationSeconds: 60
status:
  conditions:
    - type: ManagedClusterConditionAvailable
      status: "True"
  version:
    kubernetes: v1.28.0
```

**Key Labels:**
| Label | Description |
|-------|-------------|
| `openshift-virtualization` | Mark clusters with Virtualization enabled. |
| `upgrade-wave` | Group clusters for staged upgrades. |
| `cluster-name` | Unique cluster identifier. |
| `maintenance-window` | Mark cluster for scheduled maintenance. |

**Reference:** https://open-cluster-management.io/concepts/managedcluster/

---

### Placement (cluster.open-cluster-management.io/v1beta1)

Selects managed clusters based on labels and predicates.

```yaml
apiVersion: cluster.open-cluster-management.io/v1beta1
kind: Placement
metadata:
  name: virt-enabled-clusters
  namespace: openshift-virtualization-policies
spec:
  numberOfClusters: 0  # 0 = all matching clusters
  predicates:
    - requiredClusterSelector:
        labelSelector:
          matchLabels:
            openshift-virtualization: enabled
          matchExpressions:
            - key: cluster-name
              operator: In
              values:
                - cluster-a
                - cluster-b
  tolerations:
    - key: "cluster.open-cluster-management.io/unreachable"
      operator: Exists
  clusterDecisionStrategy:
    groupStrategy:
      clustersPerDecisionGroup: 1
```

**Key Spec Fields:**
| Field | Type | Description |
|-------|------|-------------|
| `numberOfClusters` | integer | Number of clusters to select (0 = all). |
| `predicates` | array | Label selectors and expressions. |
| `tolerations` | array | Tolerate cluster taints. |
| `clusterDecisionStrategy` | object | Group clusters for staged rollout. |

**PlacementDecision (Generated):**
```yaml
apiVersion: cluster.open-cluster-management.io/v1beta1
kind: PlacementDecision
metadata:
  name: virt-enabled-clusters-decision-1
  namespace: openshift-virtualization-policies
  labels:
    cluster.open-cluster-management.io/placement: virt-enabled-clusters
status:
  decisions:
    - clusterName: cluster-a
      reason: "cluster selected"
    - clusterName: cluster-b
      reason: "cluster selected"
```

**Reference:** https://open-cluster-management.io/concepts/placement/

---

### PlacementBinding (policy.open-cluster-management.io/v1)

Binds a Policy to a Placement.

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
```

**Key Fields:**
| Field | Description |
|-------|-------------|
| `placementRef` | Reference to Placement resource. |
| `subjects` | List of Policies to bind. |

**Reference:** https://open-cluster-management.io/concepts/policy/#placement-binding

---

## ACM Governance CRDs

### Policy (policy.open-cluster-management.io/v1)

Root policy object that contains one or more policy templates.

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
  automation:
    - name: aap-vm-migration-workflow
      type: Ansible
      extra_vars:
        target_node: "{{ .Object.metadata.name }}"
```

**Key Spec Fields:**
| Field | Type | Description |
|-------|------|-------------|
| `remediationAction` | string | `enforce` or `inform`. |
| `disabled` | boolean | Enable/disable policy. |
| `policy-templates` | array | List of policy types (ConfigurationPolicy, etc.). |
| `automation` | array | Automation to trigger on violation. |

**Remediation Actions:**
| Action | Behavior |
|--------|----------|
| `inform` | Report compliance only. |
| `enforce` | Attempt to remediate automatically. |

**Automation Types:**
| Type | Description |
|------|-------------|
| `Ansible` | Trigger AAP Job Template. |

**Reference:** https://open-cluster-management.io/concepts/policy/

---

### ConfigurationPolicy (policy.open-cluster-management.io/v1)

Enforces presence/absence of Kubernetes resources.

```yaml
apiVersion: policy.open-cluster-management.io/v1
kind: ConfigurationPolicy
metadata:
  name: migration-policy-config
spec:
  remediationAction: enforce
  severity: medium
  namespaceSelector:
    include:
      - openshift-cnv
    exclude:
      - kube-system
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
    - complianceType: mustnothave
      objectDefinition:
        apiVersion: v1
        kind: Pod
        metadata:
          name: forbidden-pod
          namespace: openshift-cnv
```

**Compliance Types:**
| Type | Behavior |
|------|----------|
| `musthave` | Object must exist and match spec. |
| `mustnothave` | Object must not exist. |
| `shouldhave` | Object should exist (inform only). |

**Reference:** https://open-cluster-management.io/concepts/policy/#configuration-policy

---

## AnsibleJob CRD

### AnsibleJob (tower.ansible.com/v1alpha1)

Triggers an AAP Job Template from Kubernetes.

```yaml
apiVersion: tower.ansible.com/v1alpha1
kind: AnsibleJob
metadata:
  name: trigger-vm-migration
  namespace: openshift-virtualization-policies
spec:
  tower_auth_secret: aap-credential-secret
  job_template_name: vm-migration-upgrade-workflow
  extra_vars:
    target_node: "worker-01"
    cluster_name: "cluster-a"
    maintenance_window_id: "CHG-12345"
  job_tags: "migration,drain"
  skip_tags: "post-validate"
  inventory: "openshift-clusters"
  limit: "cluster-a"
```

**Key Spec Fields:**
| Field | Type | Description |
|-------|------|-------------|
| `tower_auth_secret` | string | Name of Secret containing AAP URL and token. |
| `job_template_name` | string | Name of AAP Job Template or Workflow. |
| `extra_vars` | object | Variables passed to playbook. |
| `job_tags` | string | Comma-separated tags to run. |
| `skip_tags` | string | Comma-separated tags to skip. |
| `inventory` | string | AAP Inventory name override. |
| `limit` | string | Limit execution to specific hosts. |

**Status Fields:**
```yaml
status:
  status: successful
  job: 42
  url: https://aap-controller/api/v2/jobs/42/
  started: "2024-01-15T10:00:00Z"
  finished: "2024-01-15T10:15:00Z"
  message: "Job completed successfully"
```

**Status Values:**
| Status | Description |
|--------|-------------|
| `pending` | Job queued. |
| `running` | Job executing. |
| `successful` | Job completed. |
| `failed` | Job failed. |
| `canceled` | Job canceled. |

**Reference:** https://docs.ansible.com/automation-controller/latest/html/controlleruser/ansible_tower.html#ansible-job-resource

---

## AAP Credential Secret

Secret containing AAP Controller connection details.

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

**Required Keys:**
| Key | Description |
|-----|-------------|
| `host` | AAP Controller URL. |
| `token` | AAP API token with workflow launch permissions. |

---

## ApplicationSet CRD (GitOps)

### ApplicationSet (argoproj.io/v1alpha1)

Deploys applications to multiple clusters via GitOps.

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

**Generator Types:**
| Generator | Description |
|-----------|-------------|
| `clusterDecisionResource` | Use ACM Placement decisions. |
| `clusters` | Use ArgoCD cluster secrets. |
| `git` | Generate from Git directory. |
| `list` | Static list of clusters. |

**Reference:** https://argo-cd.readthedocs.io/en/stable/operator-manual/applicationset/

---

## External Documentation Links

### ACM Documentation

| Topic | Link |
|-------|------|
| ACM Overview | https://access.redhat.com/documentation/en-us/red_hat_advanced_cluster_management_for_kubernetes |
| Managed Clusters | https://open-cluster-management.io/concepts/managedcluster/ |
| Placement | https://open-cluster-management.io/concepts/placement/ |
| Governance Framework | https://open-cluster-management.io/concepts/policy/ |
| ConfigurationPolicy | https://open-cluster-management.io/concepts/policy/#configuration-policy |
| Policy Automation | https://docs.openshift.com/container-platform/latest/security/policy_autonomy/policy-automation.html |

### ACM-AAP Integration

| Topic | Link |
|-------|------|
| AnsibleJob Resource | https://docs.ansible.com/automation-controller/latest/html/controlleruser/ansible_tower.html#ansible-job-resource |
| Policy-Triggered Automation | https://docs.openshift.com/container-platform/latest/security/policy_autonomy/policy-automation.html |
| ACM-AAP Integration Guide | https://access.redhat.com/documentation/en-us/red_hat_advanced_cluster_management_for_kubernetes/2.8/html/integrating_with_ansible_automation_platform/index |

### OpenShift GitOps

| Topic | Link |
|-------|------|
| OpenShift GitOps | https://docs.openshift.com/container-platform/latest/gitops/understanding_red_hat_openshift_gitops.html |
| ApplicationSet | https://argo-cd.readthedocs.io/en/stable/operator-manual/applicationset/ |
| ACM + GitOps Integration | https://open-cluster-management.io/concepts/application/#deploy-application-resources-using-applicationset |

### OpenShift Virtualization

| Topic | Link |
|-------|------|
| Live Migration | https://docs.openshift.com/container-platform/latest/virt/virtual_machines/virt-node-maintenance-virt.html |
| MigrationPolicy | https://docs.openshift.com/container-platform/latest/virt/virtual_machines/virt-node-maintenance-virt.html#virt-configuring-live-migration-policies_virt-node-maintenance-virt |

---

## Sample AnsibleJob Trigger from Policy

When a policy violates, ACM can create an AnsibleJob automatically:

**Policy with Automation:**
```yaml
apiVersion: policy.open-cluster-management.io/v1
kind: Policy
metadata:
  name: node-maintenance-policy
spec:
  remediationAction: inform
  policy-templates:
    - objectDefinition:
        apiVersion: policy.open-cluster-management.io/v1
        kind: ConfigurationPolicy
        metadata:
          name: check-node-maintenance-label
        spec:
          remediationAction: inform
          object-templates:
            - complianceType: musthave
              objectDefinition:
                apiVersion: v1
                kind: Node
                metadata:
                  labels:
                    maintenance-required: "false"
  automation:
    - name: vm-migration-upgrade-workflow
      type: Ansible
      extra_vars:
        target_node: "{{ .Object.metadata.name }}"
        cluster_name: "{{ .ManagedClusterName }}"
        violation_type: "{{ .PolicyViolationType }}"
```

**Template Variables:**
| Variable | Description |
|----------|-------------|
| `{{ .Object }}` | The violating object. |
| `{{ .ManagedClusterName }}` | Name of the managed cluster. |
| `{{ .PolicyViolationType }}` | Type of violation. |
| `{{ .PolicyName }}` | Name of the policy. |

---

## Common Troubleshooting Commands

```bash
# List managed clusters
oc get managedclusters

# Check cluster connection
oc describe managedcluster <name>

# List placements
oc get placements -A

# Check placement decisions
oc get placementdecisions -n <namespace> -o yaml

# List policies
oc get policy -A

# Check policy compliance status
oc get configurationpolicy -A

# Describe policy on managed cluster
oc describe configurationpolicy <name> -n <namespace>

# List AnsibleJobs
oc get ansiblejob -A

# Check AnsibleJob status
oc get ansiblejob <name> -o yaml

# Check AnsibleJob logs (via AAP)
curl https://<aap>/api/v2/jobs/<id>/stdout/ -H "Authorization: Bearer <token>"

# View policy events
oc get events -n <namespace> --field-selector involvedObject.kind=Policy
```
