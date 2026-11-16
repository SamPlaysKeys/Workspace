# Running Discussion

## 2026-11-16 00:00 - Initial Framing & Architectural Options
Explored the conversion of a 2-drive mirrored RAID array from pod logs to ephemeral pod storage.

Key architectural distinction identified:
1. **Native `emptyDir` backed by Kubelet root (`/var/lib/kubelet`)**:
   - `emptyDir` does not natively use a Kubernetes `StorageClass`.
   - By default, `emptyDir` writes to `/var/lib/kubelet/pods/<pod-uid>/volumes/kubernetes.io~empty-dir/<vol-name>`.
   - If `/var/lib/kubelet` lives on `/`, root fills up.
   - Option A: Format/mount the mirrored RAID filesystem directly at `/var/lib/kubelet` (or bind-mount `/var/lib/kubelet/pods`). All native `emptyDir` volumes automatically land on the RAID array with zero changes to pod specs.
2. **Kubernetes StorageClass via Generic Ephemeral Volumes (PVC template inline)**:
   - Uses `Local Persistence Volume Provisioner` (or Local Storage Operator if OpenShift / TopoLVM / hostpath-provisioner).
   - Exposes a `StorageClass` (e.g. `local-ephemeral-raid`).
   - Pods use `spec.volumes[*].ephemeral.volumeClaimTemplate` referencing the storage class.
   - Lifecycle is tied to the pod (created when pod is created, deleted when pod is deleted).
3. **Local CSI Ephemeral Inline Driver**:
   - CSI driver supporting inline volume mounts directly against host local paths.
4. **Impact on Pod Logs**:
   - Pod logs will need a designated path (e.g., back onto root/var-log or another log rotation policy) if the array is repurposed.

## 2026-11-16 00:15 - Splitting the Mirrored RAID (Logs + Kubelet Ephemeral Storage)
User proposed splitting the array into two directories/mounts: one for logs (`/var/log`) and one for storage (`/var/lib/kubelet`).

Evaluated feasibility and failure modes:
- **Feasibility**: High. Completely eliminates host root filesystem disk pressure from both logs and ephemeral pod scratch volumes.
- **Key Risk with Single Filesystem Bind-Mounts**:
  - Runaway pod logs can fill pod storage.
  - Kubelet `nodefs` eviction treats them as the same device, causing pod eviction cascades even if a pod didn't breach its own ephemeral storage limit.
  - Mirrored RAID 1 write performance bottleneck under concurrent heavy logging + heavy emptyDir writes.
- **Recommended Implementation**:
  - Layer **LVM** on top of the RAID device (`/dev/mdX` or hardware RAID device).
  - Create two distinct Logical Volumes (`lv_log` and `lv_kubelet`).
  - Provides hard quota boundaries, independent filesystem tuning (e.g. XFS project quotas for fast emptyDir tracking), and predictable Kubelet eviction accounting.
- **Migration Gotchas**:
  - Requires draining node, stopping journald/containerd/kubelet, rsyncing existing data, updating fstab, and restoring SELinux contexts (`var_log_t` and `container_var_lib_t`).
- Detailed evaluation written to `wip/ephemeral-storage-migration/scratch/split-storage-evaluation.md`.

## 2026-11-16 00:30 - Proof of Concept & Ansible Automation
Created full proof of concept artifacts and production-grade Ansible automation in quarantine:
- `scratch/poc-migration-runbook.md`: Manual verification commands, node drain, staging sync, fstab, SELinux, and a verification test pod creating 2GB emptyDir files to validate root isolation.
- `ansible/vars.yml`, `ansible/inventory.ini`, `ansible/playbook.yml`: End-to-end automation playbook covering packages, node drain, service pause, LVM PV/VG/LV creation, filesystem formatting, staging and data sync, persistent mounts, SELinux labeling, and service restore.
