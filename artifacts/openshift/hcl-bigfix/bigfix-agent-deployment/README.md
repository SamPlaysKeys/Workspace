# Artifact: BigFix Client / Agent GitOps Assets

This folder contains reusable Helm components and GitOps environment wiring files for deploying the **HCL BigFix Client Host Agent** to an OpenShift cluster using Argo CD.

This implementation rigorously adheres to and matches the core component structures and conventions in the **[helm-component-pattern](artifacts/openshift/helm-component-pattern/README.md)** artifact collection.

The agent runs as a privileged node-level `DaemonSet` on every OpenShift node (including control-plane nodes) to allow host OS auditing, inventory, and patching.

## Directory Structure

```text
artifacts/openshift/bigfix-agent-deployment/
├── bootstrap/
│   └── helm-values/
│       └── applications.yaml          # App-of-Apps registration (sync-waves)
├── components/
│   └── hcl-bigfix-agent/              # Namespace, ServiceAccount, SCC Binding, DaemonSet
├── groups/
│   └── lab/
│       └── values.yaml                # Environment enablement values
└── README.md                          # This file
```

## How to Deploy

To register and activate this component in your Argo CD GitOps repository:

1. **Register the Application**:
   Add the application defined in `bootstrap/helm-values/applications.yaml` to your master `applications.yaml` under `availableApplications:`.
2. **Enable in Environments**:
   Merge the contents of `groups/lab/values.yaml` into your target environment's values file to enable the agent workload.
3. **Configure Masthead**:
   Supply your organization's PEM-encoded `actionsite.afxm` masthead in `components/hcl-bigfix-agent/values.yaml` or through Vault integration using `ExternalSecrets`.

## Detailed Guide

For full architecture details, OpenShift security contexts (SCCs), and privileged host access, refer to the prose guide:
* **[docs/guides/openshift/bigfix-agent-deployment.md](bigfix-agent-deployment.md)**
