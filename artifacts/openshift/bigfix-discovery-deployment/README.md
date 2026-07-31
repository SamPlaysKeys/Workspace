# Artifact: BigFix Inventory (BFI) Discovery & NFD GitOps Assets

This folder contains reusable Helm components and GitOps environment wiring files for deploying the **Red Hat Node Feature Discovery (NFD) Operator** and the **HCL BigFix Discovery Integration** to an OpenShift cluster using Argo CD. This is designed for Virtual Machine and licensing sub-capacity node discovery.

This implementation rigorously adheres to and matches the core component structures and conventions in the **[helm-component-pattern](../../../artifacts/helm-component-pattern/README.md)** artifact collection.

## Directory Structure

```text
artifacts/openshift/bigfix-discovery-deployment/
├── bootstrap/
│   └── helm-values/
│       └── applications.yaml          # App-of-Apps registration (sync-waves)
├── components/
│   ├── hcl-bigfix-discovery/          # ServiceAccount, RBAC, API Token, NodeFeatureRule
│   └── openshift-nfd/
│       ├── operator/                  # NFD Operator Subscription & Namespace
│       └── instance/                  # NodeFeatureDiscovery Custom Resource
├── groups/
│   └── lab/
│       └── values.yaml                # Environment enablement values
└── README.md                          # This file
```

## How to Deploy

To register and activate these components in your Argo CD GitOps repository:

1. **Register the Applications**:
   Add the applications defined in `bootstrap/helm-values/applications.yaml` to your master `applications.yaml` under `availableApplications:`.
2. **Enable in Environments**:
   Merge the contents of `groups/lab/values.yaml` into your target environment's values file to enable the operator, the instance, and the BFI discovery integration.
3. **Argo CD Sync Sequence**:
   - **Sync Wave 2**: Automatically deploys the NFD Operator Group and Subscription.
   - **Sync Wave 3**: Deploys the `NodeFeatureDiscovery` instance custom resource.
   - **Sync Wave 4**: Deploys the `ServiceAccount`, `ClusterRoles`, manual `Secret` API token, and the `NodeFeatureRule` custom resource (which safely inherits the NFD CRD).

## Detailed Guide

For full architecture details, OpenShift security postures, and sub-capacity licensing calculations, refer to the prose guide:
* **[docs/guides/openshift/bigfix-discovery-deployment.md](../../../docs/guides/openshift/bigfix-discovery-deployment.md)**
