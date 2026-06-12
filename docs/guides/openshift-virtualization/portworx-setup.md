---
type: Guide
status: Active
system: OpenShift Virtualization
related_to:
  - Portworx
  - StorageClass
  - OperatorHub
references:
  - https://docs.portworx.com/portworx-enterprise/install-portworx/openshift
---

# Configuring Portworx for OpenShift Virtualization

## Overview

Portworx by Pure Storage provides a highly available, cloud-native storage layer that integrates seamlessly with OpenShift Virtualization. It allows you to provision `ReadWriteMany` (RWX) and `ReadWriteOnce` (RWO) volumes for your virtual machines.

This guide covers deploying the Portworx Operator and configuring a basic `StorageCluster` using both the Web Console and CLI.

## Prerequisites

- OpenShift cluster with cluster-admin privileges
- Worker nodes with unformatted, raw block devices (or cloud provider volumes) attached for Portworx to consume
- Portworx Enterprise License or Trial

## 1. Install the Portworx Operator

The Operator manages the lifecycle of Portworx on your cluster.

### Web Console
1. Navigate to **Operators → OperatorHub**.
2. Search for **Portworx Enterprise**.
3. Click the Portworx tile and select **Install**.
4. Installation Mode: **All namespaces on the cluster** (default).
5. Installed Namespace: **kube-system** or **portworx** (Operator recommended defaults).
6. Click **Install** and wait for the status to show `Succeeded`.

### CLI

```yaml
# portworx-operator.yaml
apiVersion: v1
kind: Namespace
metadata:
  name: portworx
---
apiVersion: operators.coreos.com/v1
kind: OperatorGroup
metadata:
  name: portworx-operator
  namespace: portworx
spec:
  targetNamespaces:
  - portworx
---
apiVersion: operators.coreos.com/v1alpha1
kind: Subscription
metadata:
  name: portworx
  namespace: portworx
spec:
  channel: stable
  installPlanApproval: Automatic
  name: portworx
  source: certified-operators
  sourceNamespace: openshift-marketplace
```

```bash
oc apply -f portworx-operator.yaml
oc get csv -n portworx -w
```
*Wait for the CSV phase to reach `Succeeded`.*

## 2. Deploy the StorageCluster

The `StorageCluster` custom resource defines how Portworx consumes the underlying disks and configures its networking. 

*Note: You can generate a custom spec from [PX-Central](https://central.portworx.com/). The below is a generic example using available unformatted drives.*

### Web Console
1. Navigate to **Operators → Installed Operators**.
2. Select **Portworx Enterprise** in your chosen namespace.
3. Go to the **StorageCluster** tab and click **Create StorageCluster**.
4. Configure the spec (or paste the YAML generated from PX-Central). 
   - Ensure you specify how drives are consumed (e.g., `useAllWithPartitions: false`).
5. Click **Create**.

### CLI

```yaml
# portworx-cluster.yaml
apiVersion: core.libopenstorage.org/v1
kind: StorageCluster
metadata:
  name: px-cluster
  namespace: portworx
spec:
  image: portworx/oci-monitor:3.1.0 # Use the version recommended by PX-Central
  kvdb:
    internal: true
  storage:
    useAll: true # Consumes all unformatted drives on nodes
  network:
    dataInterface: eth0 # Update based on your node network interface
    mgmtInterface: eth0
  stork:
    enabled: true
```

```bash
oc apply -f portworx-cluster.yaml
```

## 3. Verify the Installation

Ensure Portworx pods are running and the cluster is operational.

### CLI
```bash
# Check pod status
oc get pods -n portworx -l name=portworx

# Check StorageCluster status
oc get storagecluster -n portworx px-cluster -o jsonpath='{.status.phase}'

# Enter a Portworx pod to check cluster status directly
oc exec -it $(oc get pods -l name=portworx -n portworx -o jsonpath='{.items[0].metadata.name}') -n portworx -- /opt/pwx/bin/pxctl status
```

**Expected:** The `pxctl status` output should show `Status: PX is operational` and list the storage nodes.

## 4. Default Storage Classes

Once installed, Portworx creates several default `StorageClass` objects. For OpenShift Virtualization, you typically want to use a class that supports RWX if you intend to do live migrations.

To verify available storage classes:
```bash
oc get sc | grep portworx
```

*Common Portworx Storage Classes:*
- `px-csi-db`: For RWO block storage (optimized for databases).
- `px-csi-sc-rwx`: For RWX shared filesystem storage.

## Common Pitfalls

| Symptom | Likely Cause | Fix |
|---|---|---|
| Portworx pods stuck in `Init` | Nodes lack unformatted block devices | Check `lsblk` on nodes. Ensure disks are unformatted and available. |
| Portworx fails to start | Network interface mismatch | Ensure `dataInterface` and `mgmtInterface` in the StorageCluster match your node topology. |
| VMs fail to live migrate | Using `ReadWriteOnce` (RWO) storage class | Ensure the VM's PVC uses a `ReadWriteMany` (RWX) Portworx StorageClass. |
