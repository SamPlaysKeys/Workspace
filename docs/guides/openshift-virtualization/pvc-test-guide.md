---
type: Guide
status: Active
system: OpenShift Virtualization
related_to:
  - PersistentVolume
  - PersistentVolumeClaim
  - StorageClass
references:
  - https://github.com/tosin2013/openshiftv4-devday/blob/master/15.UsingPersistentStorage.adoc - Reference guide for PVC workflow
---

# PVC Testing Walkthrough — OpenShift Virtualization

## Overview

Container storage is ephemeral — data disappears when a pod restarts. A
**PersistentVolumeClaim (PVC)** requests durable storage that survives pod
cycles. The cluster binds it to a **PersistentVolume (PV)** — either
pre-created by an admin or dynamically provisioned via a **StorageClass** (the
typical day-2 path).

This guide validates that PVC provisioning, mounting, and data persistence work
on your cluster before layering VM images or workloads on top.

## Prerequisites

- OpenShift cluster access — `oc` CLI **or** Web Console URL + credentials
- A `StorageClass` capable of dynamic provisioning
  (`oc get storageclass` to confirm)

## Setup

### 1. Create a Project

Isolate test resources.

**CLI:**
```bash
oc new-project pvc-test
```

**Web Console:**
**Home → Project → Create Project** → Name: `pvc-test` → **Create**

### 2. Create a PVC

Requests 1 GiB of `ReadWriteOnce` block storage.

**CLI:**

```yaml
---
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: my-claim
spec:
  accessModes:
    - ReadWriteOnce
  resources:
    requests:
      storage: 1Gi
```

```bash
oc apply -f pvc-claim.yaml
oc get pvc -w
```

Wait for `STATUS: Bound`.

**Web Console:**
**Storage → PersistentVolumeClaims → Create PVC** → Name: `my-claim`,
Size: `1 GiB`, Access mode: `ReadWriteOnce` → **Create**

### 3. Mount the PVC in a Pod

Deploy a container that sleeps indefinitely — exec in to test the mount.
Image: `quay.io/centos/centos:stream9`.

**CLI:**

```yaml
---
apiVersion: apps/v1
kind: Deployment
metadata:
  name: pv-deploy
  labels:
    app: mypv
spec:
  replicas: 1
  selector:
    matchLabels:
      app: mypv
  template:
    metadata:
      labels:
        app: mypv
    spec:
      containers:
      - name: shell
        image: quay.io/centos/centos:stream9
        command:
        - "/bin/bash"
        - "-c"
        - "sleep 10000"
        volumeMounts:
        - name: mypd
          mountPath: "/tmp/persistent"
      volumes:
      - name: mypd
        persistentVolumeClaim:
          claimName: my-claim
```

```bash
oc apply -f pvc-deploy.yaml
oc get pods -w
```

Wait for `Running`.

**Web Console:**
**Workloads → Deployments → Create Deployment** → Name: `pv-deploy`,
Image: `quay.io/centos/centos:stream9`,
Command: `["/bin/bash", "-c", "sleep 10000"]`,
**Add Volume** → `Use existing claim` → `my-claim`, Mount path: `/tmp/persistent`
→ **Create**

### 4. Write Data and Verify

**CLI:**
```bash
oc exec -it deployment/pv-deploy -- /bin/bash

# Inside container
df -h /tmp/persistent
echo "hello persistent world" > /tmp/persistent/testfile
cat /tmp/persistent/testfile
exit
```

**Web Console:**
**Workloads → Pods** → Click pod → **Terminal** tab → run the same commands.

### 5. Prove Persistence

Delete the pod (Deployment recreates it), then confirm the data survived.

```bash
oc delete pod -l app=mypv
sleep 10
oc exec -it deployment/pv-deploy -- cat /tmp/persistent/testfile
```

**Expected:** `hello persistent world`

## Teardown

**CLI:**
```bash
oc delete -f pvc-deploy.yaml
oc delete -f pvc-claim.yaml
oc delete project pvc-test
```

**Web Console:**
1. **Workloads → Deployments** → delete `pv-deploy`
2. **Storage → PersistentVolumeClaims** → delete `my-claim`
3. **Home → Project** → delete `pvc-test`

## Common Pitfalls

| Symptom | Likely Cause | Fix |
|---|---|---|
| PVC stays `Pending` | No StorageClass or insufficient backing storage | `oc describe pvc my-claim`; verify StorageClass exists |
| Pod stuck `ContainerCreating` | PVC not yet `Bound` or node can't attach | `oc get pvc`; check node storage driver |
| `Permission denied` writing to mount | SELinux / fsGroup mismatch | Set `fsGroup` in pod `securityContext` |
| Data lost after pod recycle | PVC deleted accidentally or `ReclaimPolicy: Delete` | Check PV reclaim policy; don't delete PVC |

## References

- [Using Persistent Storage - openshiftv4-devday](https://github.com/tosin2013/openshiftv4-devday/blob/master/15.UsingPersistentStorage.adoc) — Original reference guide for the PVC workflow
