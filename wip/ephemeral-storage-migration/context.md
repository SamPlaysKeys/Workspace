# Context: Ephemeral Storage Migration

**Goal**: Evaluate options and architectural changes for migrating a 2-drive mirrored RAID array (currently dedicated to pod logs) to serve as ephemeral pod storage (relieving host root directory pressure) for a dev environment, while also evaluating a split-storage model where the array hosts both pod logs and kubelet ephemeral storage.

**Current State**:
- Two drives in a mirrored RAID array currently mounted/used for pod logs.
- Pod ephemeral volumes (`emptyDir`, etc.) currently reside on the host's root filesystem (`/`), creating risk of root disk exhaustion.
- Architecture selected for PoC: **LVM-split on mirrored RAID** (`lv_log` -> `/var/log`, `lv_kubelet` -> `/var/lib/kubelet`).
- PoC runbook drafted in `wip/ephemeral-storage-migration/scratch/poc-migration-runbook.md`.
- Automated Ansible playbook and variables built in `wip/ephemeral-storage-migration/ansible/`.

**Key Findings & Recommendations**:
- Splitting the array between logs and `/var/lib/kubelet` via LVM guarantees blast radius containment, accurate Kubelet `nodefs` evictions, and online volume resizing.
- Formatted `lv_kubelet` with `xfs` and `pquota` mount option to optimize Kubelet ephemeral tracking.
- Ansible automation handles node drain, service pause, LVM provisioning, data rsync staging, persistent fstab mounting, SELinux relabeling, and service restore.

**Artifacts Generated in Quarantine**:
- `scratch/split-storage-evaluation.md`: Technical analysis of shared vs LVM split.
- `scratch/poc-migration-runbook.md`: Step-by-step manual PoC commands and verification pod.
- `ansible/vars.yml`: Configurable parameters (device, LV sizes, mount options).
- `ansible/inventory.ini`: Sample host inventory.
- `ansible/playbook.yml`: End-to-end automated migration playbook.
