# Plan: HCL BigFix Discovery (bfi) Component

## Goal

Configure the OpenShift cluster to integrate with an external HCL BigFix Inventory (BFI) Server for Virtual Machine discovery and IBM sub-capacity licensing (PVU) auditing. 

This requires:
1. Installing the Red Hat **Node Feature Discovery (NFD) Operator** to detect and label hardware socket topology.
2. Creating a dedicated **ServiceAccount** with RBAC to allow the external BFI Server to query the cluster APIs.
3. Defining a **NodeFeatureRule** to label OpenShift nodes with BFI-compatible hardware topology metadata (`ibm.ilmt.*`).

We will deploy this using a two-part Helm/GitOps approach to respect Custom Resource Definition (CRD) lifecycles, targeting **lab first**.

---

## Component 1: Node Feature Discovery (NFD) Operator (`components/openshift-nfd/`)

This component manages the installation of the NFD Operator.

### Files to create:
1. **`Chart.yaml`** — `name: openshift-nfd`, `version: 1.0.0`
2. **`values.yaml`** — Parameterized values for channel (e.g., `stable`), starting CSV, and target namespace.
3. **`templates/namespace.yaml`** — Creates the `openshift-nfd` namespace.
4. **`templates/operatorgroup.yaml`** — Standard `OperatorGroup` pointing to `openshift-nfd`.
5. **`templates/subscription.yaml`** — Standard OLM Subscription for `nfd` sourced from `redhat-operators`.

---

## Component 2: BFI Discovery Integration (`components/hcl-bigfix-discovery/`)

This component establishes API access and hardware labels.

### Files to create:
1. **`Chart.yaml`** — `name: hcl-bigfix-discovery`, `version: 1.0.0`
2. **`values.yaml`** — Parameterized configurations:
   - `namespace.name: bigfix`
   - `serviceAccount.name: big-fix-serviceaccount`
3. **`templates/namespace.yaml`** — Creates the `bigfix` namespace.
4. **`templates/sa.yaml`** — Creates `big-fix-serviceaccount`.
5. **`templates/secret-token.yaml`** — Manually defines a long-lived ServiceAccount token Secret (using the `kubernetes.io/service-account-token` annotation required in OpenShift 4.11+).
6. **`templates/rbac.yaml`** — Creates the following ClusterRoles and bindings:
   - `bfi-cluster-nodes-list` (permits `list` on `nodes`)
   - `bfi-cluster-virtualmachineinstance-list` (permits `list`/`get` on `virtualmachineinstances.kubevirt.io` for OpenShift Virtualization VMs)
   - `bfi-cluster-namespaces-list` (permits `list`/`get` on `namespaces`)
7. **`templates/nodefeaturerule.yaml`** — Creates the `NodeFeatureRule` Custom Resource (`ibm.ilmt.capacity-sockets`) to template CPU socket topology into BFI-compatible node labels.

---

## Wiring changes

8. **`bootstrap/helm-values/applications.yaml`** — Register both applications under `availableApplications:`:
   ```yaml
   openshift-nfd:
     annotations:
       argocd.argoproj.io/sync-wave: '2' # Phase 1: Deploy Operator first
     source:
       path: components/openshift-nfd
   
   hcl-bigfix-discovery:
     annotations:
       argocd.argoproj.io/sync-wave: '4' # Phase 2: Deploy CRD-dependent resources later
     source:
       path: components/hcl-bigfix-discovery
   ```

9. **`groups/lab/values.yaml`** — Enable both components:
   ```yaml
   openshift-nfd: {}
   hcl-bigfix-discovery: {}
   ```
