---
type: Guide
status: Active
system: OpenShift Virtualization
related_to:
  - Portworx
  - PersistentVolumeClaim
  - ReadWriteMany
references:
---

# Portworx PVC Testing Walkthrough (RWX)

## Overview

When using OpenShift Virtualization, Virtual Machine live migrations require shared storage. Portworx provides this via `ReadWriteMany` (RWX) storage classes.

This guide validates that Portworx RWX provisioning works using the `px-rwx-vm` StorageClass. To prove it's truly shared storage, we will deploy **two** pods attached to the same PVC, write data from one pod, and read it from the other.

## Prerequisites

- Portworx installed and operational (`pxctl status` shows operational)
- The `px-rwx-vm` StorageClass exists on the cluster
- OpenShift cluster access (`oc` CLI or Web Console)

## Setup

### 1. Create a Project

**CLI:**
```bash
oc new-project px-pvc-test
```

**Web Console:**
**Home → Project → Create Project** → Name: `px-pvc-test` → **Create**

### 2. Create the RWX PVC

Notice we specify `storageClassName: px-rwx-vm` and `accessModes: ReadWriteMany`.

**CLI:**

```yaml
---
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: px-shared-claim
spec:
  storageClassName: px-rwx-vm
  accessModes:
    - ReadWriteMany
  resources:
    requests:
      storage: 2Gi
```

```bash
oc apply -f px-claim.yaml
oc get pvc -w
```

Wait for `STATUS: Bound`.

**Web Console:**
**Storage → PersistentVolumeClaims → Create PVC** → Name: `px-shared-claim`,
StorageClass: `px-rwx-vm`, Size: `2 GiB`, Access mode: `Shared Access (RWX)` → **Create**

### 3. Deploy Multiple Pods Sharing the PVC

We will scale a deployment to 2 replicas. Both pods will mount the exact same volume.

**CLI:**

```yaml
---
apiVersion: apps/v1
kind: Deployment
metadata:
  name: px-shared-deploy
  labels:
    app: px-test
spec:
  replicas: 2
  selector:
    matchLabels:
      app: px-test
  template:
    metadata:
      labels:
        app: px-test
    spec:
      containers:
      - name: shell
        image: quay.io/centos/centos:stream9
        command:
        - "/bin/bash"
        - "-c"
        - "sleep 10000"
        volumeMounts:
        - name: shared-data
          mountPath: "/data/px-shared"
      volumes:
      - name: shared-data
        persistentVolumeClaim:
          claimName: px-shared-claim
```

```bash
oc apply -f px-deploy.yaml
oc get pods -w
```

Wait for both pods to show `Running`.

**Web Console:**
**Workloads → Deployments → Create Deployment** → Name: `px-shared-deploy`, Replicas: `2`,
Image: `quay.io/centos/centos:stream9`, Command: `["/bin/bash", "-c", "sleep 10000"]`,
**Add Volume** → `Use existing claim` → `px-shared-claim`, Mount path: `/data/px-shared` → **Create**

### 4. Prove Shared Access (Write on Pod A, Read on Pod B)

**CLI:**

1. Get the names of both pods:
```bash
oc get pods -l app=px-test
```
*(You will see two pods, e.g., `px-shared-deploy-xxxx` and `px-shared-deploy-yyyy`)*

2. Write a file from the first pod:
```bash
oc exec -it <POD_A_NAME> -- bash -c 'echo "Portworx RWX Test: $(date)" > /data/px-shared/live-migration-test.txt'
```

3. Instantly read it from the second pod:
```bash
oc exec -it <POD_B_NAME> -- cat /data/px-shared/live-migration-test.txt
```

**Expected:** The second pod outputs the exact timestamp written by the first pod. This validates the shared storage is functioning, meaning VM live migrations will work.

## Teardown

**CLI:**
```bash
oc delete -f px-deploy.yaml
oc delete -f px-claim.yaml
oc delete project px-pvc-test
```

**Web Console:**
1. **Workloads → Deployments** → delete `px-shared-deploy`
2. **Storage → PersistentVolumeClaims** → delete `px-shared-claim`
3. **Home → Project** → delete `px-pvc-test`

## Common Pitfalls

| Symptom | Likely Cause | Fix |
|---|---|---|
| PVC stays `Pending` | `px-rwx-vm` StorageClass is missing or typoed | `oc get sc` to verify the exact name of the Portworx RWX class. |
| Second pod stuck in `ContainerCreating` | StorageClass does not actually support RWX | Ensure the underlying SC provisioner supports shared volumes, not just block RWO. |
| Files written on Pod A don't appear on Pod B | Not using RWX or mounted differently | Verify both pods have the `volumeMount` pointing to the same `claimName`. |
