# Architectural Evaluation: Splitting Mirrored RAID for Logs & Kubelet

## Executive Summary
Repurposing the mirrored RAID array to host **both** pod logs (e.g. `/var/log` or `/var/log/pods`) and ephemeral storage (`/var/lib/kubelet`) is a viable, high-value architecture. It insulates the host root filesystem (`/`) from the two most volatile write sources in container nodes: application logs and pod temporary scratch volumes.

However, **how** the array is split dictates stability, isolation, and failure modes.

---

## The Core Challenge: Why Root Disk Exhaustion Happens
1. **Default Kubelet Behavior**:
   - `emptyDir: {}` writes directly to `/var/lib/kubelet/pods/<pod-uid>/volumes/kubernetes.io~empty-dir/<name>`.
   - If `/var/lib/kubelet` resides on `/`, any large file write by a pod competes directly with OS system binaries, swap, and runtime state.
2. **Kubelet Eviction Mechanism (`nodefs`)**:
   - Kubelet monitors `nodefs.available` and `nodefs.inodesFree`.
   - When `/var/lib/kubelet` is on the root disk, log churn, container rootfs, and emptyDir all degrade the same metric, leading to rapid node eviction loops (`The node was low on resource: ephemeral-storage`).

---

## Strategy Comparison: How to Split the Array

### Approach 1: LVM Partitioning (Recommended)
Place LVM on top of the mirrored RAID device (`/dev/mdX` or hardware RAID controller device) and create two independent Logical Volumes:
- `lv_log` (e.g., 20-30% capacity or fixed 50–100GB) -> mounted to `/var/log`
- `lv_kubelet` (remaining capacity) -> mounted to `/var/lib/kubelet`

#### Advantages:
- **Hard Blast Radius Containment**: A pod with an uncontrolled logging bug that floods gigabytes of logs will fill `lv_log` without starving pods running in `lv_kubelet` (and vice-versa).
- **Accurate Kubelet Eviction**: Kubelet evaluates `nodefs` based on `/var/lib/kubelet`. Having a dedicated filesystem allows Kubelet to evict misbehaving ephemeral storage pods accurately based on pod-specific limits.
- **Dynamic Flexibility**: LVM allows online volume extension if one side needs more capacity later.
- **Filesystem Customization**: `lv_kubelet` can be formatted with `xfs` and project quotas enabled (`pquota`/`prjquota`), allowing Kubelet to track emptyDir capacity faster via quota tracking rather than recursive directory walking (`du`).

---

### Approach 2: Single Filesystem with Subdirectory Bind Mounts
Format the entire RAID array as one large filesystem (e.g., mounted at `/mnt/raid`), then bind mount:
- `/mnt/raid/log` -> `/var/log`
- `/mnt/raid/kubelet` -> `/var/lib/kubelet`

#### Risks & Gotchas:
1. **Shared Capacity (No Isolation)**: A log deluge will eat into pod scratch storage.
2. **Kubelet Eviction Confusion**: Because `/var/log` and `/var/lib/kubelet` share the same device and inode table, Kubelet's disk monitoring can trigger `DiskPressure` eviction cascades on pods even when those pods used almost zero ephemeral storage.
3. **I/O Contention on RAID 1**: In mirrored RAID 1, every write must hit both disks. If developers run high-throughput temp file tasks (e.g. database seeds, large compile caches, data extracts) while pods simultaneously write verbose debug logs, synchronous disk write bandwidth becomes a shared bottleneck.

---

## Operational Migration Gotchas

If executing this migration on a live host:

1. **Active Log Daemons**:
   - `systemd-journald`, `rsyslog`, and container runtimes (CRI-O / containerd) open log files immediately at boot.
   - Any mount over `/var/log` without pre-migrating contents will mask active system log files and boot artifacts.
   - **Procedure**: Drain node -> Stop kubelet and container engine -> Temporarily mount the new volume to `/mnt/target` -> `rsync -aHAX /var/log/ /mnt/target/` -> update `/etc/fstab` -> mount `/var/log` -> start services.

2. **Kubelet State Preservation**:
   - `/var/lib/kubelet` contains CSI registration sockets, plugin registrations, and volume mount trackers.
   - Node must be cordoned and drained before remounting `/var/lib/kubelet`.
   - Existing metadata should be cleanly synced or rebuilt cleanly on node restart.

3. **SELinux Permissions (Critical on RHEL / CentOS / Fedora / CoreOS)**:
   - Mounting new filesystems at `/var/log` and `/var/lib/kubelet` requires resetting SELinux labels:
     - `restorecon -Rv /var/log` (`var_log_t`)
     - `restorecon -Rv /var/lib/kubelet` (`container_var_lib_t`)
   - Failure to do this will cause container engines and Kubelet to fail with `Permission denied` on volume mounts and socket creation.
