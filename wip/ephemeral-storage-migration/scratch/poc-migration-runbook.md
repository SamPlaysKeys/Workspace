# PoC Runbook: Mirrored RAID Split (Logs & Kubelet Ephemeral Storage)

This Proof of Concept (PoC) validates the safe migration of an existing mirrored RAID array to host two isolated LVM logical volumes:
1. `/var/log` (System and Pod logs)
2. `/var/lib/kubelet` (Kubelet root, pod ephemeral storage & `emptyDir`)

---

## 0. Prerequisites & Variables
Assume for this PoC:
- **RAID Device**: `/dev/md0` (or hardware RAID block device like `/dev/sdb`)
- **Volume Group**: `vg_node_storage`
- **Logical Volumes**:
  - `lv_log`: 60GB (or 25-30% of total)
  - `lv_kubelet`: Remaining capacity
- **Node Name**: `node-01.dev.internal`

---

## Step 1: Cordon and Drain Node
Prevent new pods from scheduling and gracefully evict existing workloads:
```bash
# Mark unschedulable
kubectl cordon node-01.dev.internal

# Drain workloads (ignoring daemonsets)
kubectl drain node-01.dev.internal \
  --ignore-daemonsets \
  --delete-emptydir-data \
  --force
```

---

## Step 2: Stop Services and Prepare Existing Array
Stop the container runtime and logging daemons to prevent dirty writes during the sync:
```bash
# Stop Kubelet and Container Engine
systemctl stop kubelet
systemctl stop containerd   # or crio

# Stop Logging services
systemctl stop systemd-journald
systemctl stop rsyslog 2>/dev/null || true
```

Unmount the old array mount point (e.g. if it was mounted at `/mnt/old-log-array` or `/var/log/pods`):
```bash
umount /dev/md0 || true
```

---

## Step 3: Initialize LVM on RAID Device
```bash
# 1. Initialize physical volume
pvcreate /dev/md0

# 2. Create Volume Group
vgcreate vg_node_storage /dev/md0

# 3. Create Logical Volumes
# Allocate 60GB for logs
lvcreate -L 60G -n lv_log vg_node_storage

# Allocate remaining 100% free space for Kubelet & ephemeral storage
lvcreate -l 100%FREE -n lv_kubelet vg_node_storage
```

---

## Step 4: Format Filesystems
Format both volumes. For `lv_kubelet`, format with `xfs` and ensure project quotas are supported:
```bash
# Format log volume
mkfs.xfs -L var_log /dev/vg_node_storage/lv_log

# Format kubelet volume
mkfs.xfs -L var_kubelet /dev/vg_node_storage/lv_kubelet
```

---

## Step 5: Stage and Rsync Existing Data
Temporarily mount each LV and migrate the contents from the host's existing directories preserving permissions, ACLs, and extended attributes (`-aHAX`):

### 5.1 Migrate `/var/log`
```bash
mkdir -p /mnt/stage_log
mount /dev/vg_node_storage/lv_log /mnt/stage_log

# Rsync existing logs
rsync -aHAX --info=progress2 /var/log/ /mnt/stage_log/

umount /mnt/stage_log
rmdir /mnt/stage_log
```

### 5.2 Migrate `/var/lib/kubelet`
```bash
mkdir -p /mnt/stage_kubelet
mount /dev/vg_node_storage/lv_kubelet /mnt/stage_kubelet

# Rsync existing kubelet state
rsync -aHAX --info=progress2 /var/lib/kubelet/ /mnt/stage_kubelet/

umount /mnt/stage_kubelet
rmdir /mnt/stage_kubelet
```

---

## Step 6: Configure `/etc/fstab` and Mount Persistently
Add entries to `/etc/fstab` using device paths or filesystem labels:

```text
/dev/vg_node_storage/lv_log      /var/log          xfs    defaults        0 0
/dev/vg_node_storage/lv_kubelet  /var/lib/kubelet  xfs    defaults,pquota 0 0
```

Mount both filesystems:
```bash
mount /var/log
mount /var/lib/kubelet
```

Verify mounts:
```bash
df -h /var/log /var/lib/kubelet
```

---

## Step 7: Restore SELinux Labels (RHEL / Rocky / Fedora / CoreOS)
Reset security contexts on both mounts:
```bash
restorecon -Rv /var/log
restorecon -Rv /var/lib/kubelet
```

---

## Step 8: Restart Daemons & Uncordon
```bash
# Restart system journal & syslog
systemctl start systemd-journald
systemctl start rsyslog 2>/dev/null || true

# Restart container runtime and kubelet
systemctl start containerd   # or crio
systemctl start kubelet

# Verify Kubelet is Ready
systemctl status kubelet

# Uncordon node
kubectl uncordon node-01.dev.internal
```

---

## Step 9: PoC Verification Test
Deploy a test pod requesting `emptyDir` to verify that writes land on `/var/lib/kubelet` (backed by `lv_kubelet`) and do not consume root filesystem space (`/`):

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: poc-ephemeral-test
  namespace: default
spec:
  containers:
    - name: writer
      image: busybox
      command: ["sh", "-c"]
      args:
        - |
          echo "Writing 2GB test file to emptyDir..."
          dd if=/dev/zero of=/scratch/test.dat bs=1M count=2048 status=progress
          sleep 3600
      volumeMounts:
        - mountPath: /scratch
          name: ephemeral-scratch
  volumes:
    - name: ephemeral-scratch
      emptyDir: {}
```

### Verification Checks:
1. `df -h /`: Root filesystem usage remains completely unchanged.
2. `df -h /var/lib/kubelet`: Usage increases by 2GB.
3. Check pod status: Pod is running with no disk pressure warnings.
