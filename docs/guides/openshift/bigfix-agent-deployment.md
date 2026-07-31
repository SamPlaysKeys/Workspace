---
type: Guide
status: Active
system: OpenShift Container Platform
related_to: [HCL BigFix, Security Context Constraints]
references:
  - title: "HCL BigFix Client Installation Guides"
    url: "https://help.hcl-software.com/bigfix/11.0/platform/Console/Installation_Guide/c_bes_client_installation.html"
---

# BigFix Host Agent Client Deployment on OpenShift via GitOps

## Overview

This guide details the architecture, design decisions, and configuration required to deploy the **HCL BigFix Host Agent Client** onto a Red Hat OpenShift Container Platform (OCP) cluster using Argo CD.

This implementation rigorously adheres to and matches the standardized Helm component structures, sync-waves, and folder layouts defined in the **[helm-component-pattern](../../../artifacts/helm-component-pattern/README.md)** artifact collection.

The Host Agent Client runs directly on each physical/virtual cluster node. It monitors host operating system settings, hardware properties, and installed package inventories, and applies host-level configuration patches.

---

## Architecture

Because the BigFix Host Agent inspects and manages the underlying Red Hat Enterprise Linux CoreOS (RHCOS) or RHEL host OS, it must execute with root/privileged credentials.

```text
+-----------------------------------------------------------+
| OpenShift Node (RHCOS / RHEL)                             |
|                                                           |
|   +----------------------------------------------------+  |
|   | bigfix Namespace                                   |  |
|   |                                                    |  |
|   |  +--------------------+    +--------------------+  |  |
|   |  | ServiceAccount     |    | Masthead Secret    |  |  |
|   |  | bigfix-agent-sa    |    | (actionsite.afxm)  |  |  |
|   |  +---------+----------+    +---------+----------+  |  |
|   |            |                         |             |  |
|   |            v                         |             |  |
|   |  +---------+----------+              |             |  |
|   |  | SCC RoleBinding    |              |             |  |
|   |  | (privileged)       |              |             |  |
|   |  +---------+----------+              |             |  |
|   |            |                         |             |  |
|   |            v                         v             |  |
|   |  +---------+-------------------------+----------+  |  |
|   |  | hcl-bigfix-agent DaemonSet Pod                |  |  |
|   |  |   - hostPID: true                             |  |  |
|   |  |   - hostNetwork: true                         |  |  |
|   |  |   - securityContext.privileged: true          |  |  |
|   |  +----+------------------------------------+----+  |  |
|   |       |                                    |       |  |
|   +-------|------------------------------------|-------+  |
|           | (Host Volume Mounts)               |          |
|           v                                    v          |
|   +-------+--------------+             +-------+------+   |
|   | Host Root Filesystem |             | State Folder |   |
|   | / (mounted to /host) |             | /var/opt/B...|   |
|   +----------------------+             +--------------+   |
+-----------------------------------------------------------+
```

To achieve direct host-level inspection, the workload is packaged as a cluster-wide **DaemonSet** configured with:
* **`privileged: true`**: Grants absolute container permissions.
* **`hostPID: true`**: Direct visibility into the host's process tree.
* **`hostNetwork: true`**: Direct communication with the network via host IP interfaces.
* **Host Volume Mounts**: 
  - Host `/` mounted to `/host` so the agent can read/write root configuration files.
  - Host `/var/opt/BESClient` mounted to persist client database, certificates, and logging state across container rebuilds.

---

## OpenShift Security Posture & SCCs

By default, OpenShift strictly forbids containers from running as root, mounting host paths, or accessing host namespaces. To deploy a privileged host agent, you must grant the DaemonSet's `ServiceAccount` the `privileged` Security Context Constraint (SCC).

This is achieved securely in GitOps by creating a `RoleBinding` within the target namespace that binds the `ServiceAccount` to the cluster-provided `system:openshift:scc:privileged` `ClusterRole`:

```yaml
apiVersion: rbac.authorization.k8s.io/v1
kind: RoleBinding
metadata:
  name: bigfix-agent-privileged-scc
  namespace: hcl-bigfix
roleRef:
  apiGroup: rbac.authorization.k8s.io
  kind: ClusterRole
  name: system:openshift:scc:privileged
subjects:
  - kind: ServiceAccount
    name: bigfix-agent-sa
    namespace: hcl-bigfix
```

---

## Configuration Steps

The deployment is self-contained under `artifacts/openshift/bigfix-agent-deployment/`.

1. **Namespace & ServiceAccount**:
   The namespace `hcl-bigfix` is isolated with monitoring active.
2. **Masthead Secret Injection**:
   Every BigFix client requires an `actionsite.afxm` masthead file to verify keys and trace upstream Relays. This is injected as a Kubernetes `Secret` of type `Opaque`.
3. **DaemonSet Setup**:
   The DaemonSet includes explicit **tolerations** to ensure the client is deployed onto Master/Control-Plane nodes as well as worker nodes, ensuring 100% host coverage:
   ```yaml
   tolerations:
     - key: node-role.kubernetes.io/master
       operator: Exists
       effect: NoSchedule
     - key: node-role.kubernetes.io/control-plane
       operator: Exists
       effect: NoSchedule
   ```

---

## Common Pitfalls

1. **Missing Master Tolerations**:
   Without master/control-plane node tolerations, control-plane nodes will lack auditing coverage, creating a blind spot in compliance and security tracking.
2. **State Persistence**:
   If the `/var/opt/BESClient` host volume mount is omitted, the client will generate a new unique ComputerID on every pod restart. This will bloat the BigFix Console with thousands of duplicate offline entries. Always ensure this directory is mapped to a host path (using `DirectoryOrCreate`).

---

## References
* [HCL BigFix Platform Documentation](https://help.hcl-software.com/bigfix/11.0/platform/index.html) - Platform features, command guides, and client management.
