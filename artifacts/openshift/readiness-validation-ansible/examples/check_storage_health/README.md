---
type: README-Note
---

# readiness_check_storage_health

Validates the health of storage resources on a running OpenShift cluster:
PersistentVolumes, PersistentVolumeClaims, and required StorageClasses.

## What it does

1. Fetches all PersistentVolumes via `oc get pv -o json` and evaluates their phase
2. Fetches PVCs (all namespaces or a configured subset) and evaluates their phase
3. Checks that each required StorageClass exists
4. Appends a three-section report to the shared `readiness_report_md` fact

## Result statuses

### PersistentVolumes

| Status | Condition |
|--------|-----------|
| `PASS` | PV phase is in `pv_healthy_phases` (Bound or Available) |
| `WARN` | PV phase is not in `pv_healthy_phases` but not in `pv_fail_phases` |
| `FAIL` | PV phase is in `pv_fail_phases` (Failed) |

### PersistentVolumeClaims

| Status | Condition |
|--------|-----------|
| `PASS` | PVC phase is `Bound` |
| `WARN` | PVC phase is Pending or other non-Bound, non-Lost phase |
| `FAIL` | PVC phase is in `pvc_fail_phases` (Lost) |

### StorageClasses

| Status | Condition |
|--------|-----------|
| `PASS` | StorageClass exists |
| `WARN` | StorageClass absent and `required: false` |
| `FAIL` | StorageClass absent and `required: true` |

## Variables

### `pv_healthy_phases`

List of PV phases considered healthy. Default: `["Bound", "Available"]`

### `pv_fail_phases`

List of PV phases that produce `FAIL`. Default: `["Failed"]`

### `storage_classes_required`

List of StorageClass names to check:

```yaml
storage_classes_required:
  - name: "standard"
    required: false
  - name: "fast-ssd"
    required: true
```

### `pvc_check_namespaces`

List of namespaces to scan for PVCs. Empty list (default) scans all namespaces.

```yaml
pvc_check_namespaces:
  - "my-app"
  - "another-app"
```

### `pvc_fail_phases`

List of PVC phases that produce `FAIL`. Default: `["Lost"]`

## Requirements

- `oc` CLI available on target host with a valid kubeconfig
- Target host: bastion group (parent playbook sets `KUBECONFIG` at play level)
