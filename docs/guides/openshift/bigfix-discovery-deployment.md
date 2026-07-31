---
type: Guide
status: Active
system: OpenShift Container Platform
related_to: [HCL BigFix, Node Feature Discovery Operator, Argo CD]
references:
  - title: "HCL BigFix Inventory OpenShift Integration"
    url: "https://help.hcl-software.com/bigfix/11.0/inventory/Inventory/admin/red-hat-openshift-virtualization.html"
---

# BigFix Inventory (BFI) Discovery & NFD Deployment via GitOps

## Overview

This guide details the architecture, design decisions, and configuration required to deploy the **HCL BigFix Inventory (BFI)** discovery and sub-capacity licensing integration on Red Hat OpenShift Container Platform (OCP).

The BFI discovery integration is a **headless, API-level integration**. It does not install a resident server. Instead, it deploys a `ServiceAccount` and a long-lived API token that allows the external BigFix Inventory server to query OpenShift APIs. It also deploys the **Red Hat Node Feature Discovery (NFD) Operator** to map and label host CPU sockets and cores, which BFI queries to calculate Processor Value Unit (PVU) licensing.

---

## Architecture & Integration Flow

The integration bridges the external BFI server and the OpenShift Control Plane:

```text
                                           +---------------------------------+
                                           |     External BFI Server         |
                                           +----------------+----------------+
                                                            | (REST API Query)
                                                            v
+-----------------------------------------------------------+----------------+
| OpenShift Cluster                                                          |
|                                                                            |
|  +------------------------------+        +------------------------------+  |
|  | Namespace: bigfix            |        | Namespace: openshift-nfd     |  |
|  |                              |        |                              |  |
|  |  +-------------------------+ |        |  +-------------------------+ |  |
|  |  | ServiceAccount          | |        |  | NFD Operator            | |  |
|  |  | big-fix-serviceaccount  | |        |  | (Subscription)          | |  |
|  |  +------------+------------+ |        |  +------------+------------+ |  |
|  |               |              |        |               |                  |
|  |               v              |        |               v (CRD Wave 3)     |
|  |  +-------------------------+ |        |  +-------------------------+ |  |
|  |  | API Token Secret        | |        |  | NodeFeatureDiscovery    | |  |
|  |  | bfi-service-account-tok | |        |  | (Instance CR)           | |  |
|  |  +-------------------------+ |        |  +------------+------------+ |  |
|  |                              |        |               |                  |
|  +------------------------------+        |               v (Labels Wave 4)  |
|                                          |  +-------------------------+ |  |
|                                          |  | NodeFeatureRule         | |  |
|                                          |  | ibm.ilmt.capacity-sock  | |  |
|                                          |  +-------------------------+ |  |
|                                          +------------------------------+  |
+----------------------------------------------------------------------------+
```

1. **API Discovery**: The external BFI server connects via the `big-fix-serviceaccount` token to read `nodes`, `namespaces`, and `virtualmachineinstances` (to track OpenShift Virtualization VM cores).
2. **CPU Topology Labeling**: The NFD Operator discovers core and socket details on each bare-metal or virtual node, and matches them against an ILMT-specific `NodeFeatureRule` to apply standard capacity labels (e.g., `ibm.ilmt.cores`, `ibm.ilmt.sockets`). BFI queries these labels to generate compliant sub-capacity reports.

---

## GitOps Sync-Wave Strategy

Applying operators and their dependent Custom Resources (CRs) simultaneously inside GitOps can cause synchronization deadlocks because the Custom Resource Definitions (CRDs) do not exist when the CR templates are parsed.

To ensure a seamless, zero-race-condition bootstrap, we separate the operator from its CR instances and structure the workloads across **Argo CD Sync-Waves**:

| Wave | Component | Action |
|:---:|:---|:---|
| **`2`** | `components/openshift-nfd/operator` | Installs the NFD namespace, OLM OperatorGroup, and Subscription to register the CRDs. |
| **`3`** | `components/openshift-nfd/instance` | Installs the `NodeFeatureDiscovery` controller instance custom resource. |
| **`4`** | `components/hcl-bigfix-discovery` | Deploys the discovery namespace, API Token Secret, RBAC rules, and the NFD `NodeFeatureRule` CR. |

---

## Configuration Details

### RBAC Permissions for BFI Server

The external BFI Server relies on the ServiceAccount token to authenticate and call APIs. The ClusterRoles must be isolated to read-only resource lists:

```yaml
kind: ClusterRole
apiVersion: rbac.authorization.k8s.io/v1
metadata:
  name: bfi-cluster-nodes-list
rules:
  - verbs: [ "list" ]
    apiGroups: [ "" ]
    resources: [ "nodes" ]
---
kind: ClusterRole
apiVersion: rbac.authorization.k8s.io/v1
metadata:
  name: bfi-cluster-virtualmachineinstance-list
rules:
  - verbs: [ "list", "get" ]
    apiGroups: [ "kubevirt.io" ]
    resources: [ "virtualmachineinstances" ]
---
kind: ClusterRole
apiVersion: rbac.authorization.k8s.io/v1
metadata:
  name: bfi-cluster-namespaces-list
rules:
  - verbs: [ "list", "get" ]
    apiGroups: [ "" ]
    resources: [ "namespaces" ]
```

### Escaping Go Templates in Helm for NFD

The NFD `NodeFeatureRule` injects values into labels using Go template notation (`{{ }}`). Because Helm also parses Go templates, you must escape these braces in Helm source templates:

```yaml
      labelsTemplate: |
        {{ "{{" }} range .cpu.topology }}ibm.ilmt.{{ "{{" }} .Name }}={{ "{{" }} .Value }}
        {{ "{{" }} end }}
```
This forces Helm to output the braces literally, allowing the NFD operator to interpret them at runtime.

---

## Common Pitfalls

1. **ServiceAccount Tokens in OpenShift 4.11+**:
   Kubernetes 1.24+ and OpenShift 4.11+ no longer generate static long-lived token Secrets automatically when a ServiceAccount is created. You must manually define a Secret of type `kubernetes.io/service-account-token` with the ServiceAccount annotation:
   ```yaml
   apiVersion: v1
   kind: Secret
   metadata:
     name: bfi-service-account-token
     namespace: bigfix
     annotations:
       kubernetes.io/service-account.name: big-fix-serviceaccount
   type: kubernetes.io/service-account-token
   ```
2. **CRD Lifecycle Race Conditions**:
   Do not combine NFD Operators and `NodeFeatureRule` into a single Helm component. Always run them in separate waves or as distinct applications to ensure the operator is running and ready to handle the `NodeFeatureRule` Custom Resource.

---

## References
* [HCL BigFix Inventory OpenShift Integration](https://help.hcl-software.com/bigfix/11.0/inventory/Inventory/admin/red-hat-openshift-virtualization.html) - Step-by-step configurations for sub-capacity reporting.
