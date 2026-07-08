# readiness_check_root_disk_raid

Validates that each BareMetalHost root device hint resolves to inspected storage
that looks like a RAID-backed virtual disk/controller.

## What it does

1. Lists BareMetalHosts in `root_disk_raid_bmh_namespace`.
2. Optionally filters BMH names with `root_disk_raid_bmh_name_regex`.
3. For each BMH, determines the root device hint type:
   - **`spec.rootDeviceHints.wwn`** (primary) — matched against
     `status.hardware.storage[].wwn` after normalizing case, `0x` prefix, and
     non-hex characters.
   - **`spec.rootDeviceHints.deviceName`** (fallback when no WWN is set) —
     matched by exact string equality against `status.hardware.storage[].name`
     or any entry in `status.hardware.storage[].alternateNames`.  This covers
     control plane nodes whose hardware does not expose a WWN on the virtual
     RAID disk, where the hint is set to a path such as `/dev/sda` or a stable
     by-path link like `/dev/disk/by-path/pci-0000:02:00.0-scsi-0:0:0:0`.
4. Checks the matched storage model against `root_disk_raid_expected_models`.

This is intentionally lightweight. It does not use Redfish or `oc debug node`;
it relies on the BMH inspection data that operators already use for this check.

## Result statuses

| Status | Meaning |
|--------|---------|
| `PASS` | A root device hint (`wwn` or `deviceName`) matched inspected BMH storage and the matched model looked RAID-backed. |
| `WARN` | A check could not be proven and `root_disk_raid_required: false`. |
| `FAIL` | A required check could not be proven, neither hint type matched storage, or the matched model did not look RAID-backed. |

## Variables

### `root_disk_raid_bmh_namespace`

Namespace containing BareMetalHosts. Default: `openshift-machine-api`.

### `root_disk_raid_bmh_name_regex`

Optional regex applied to BMH names before evaluation. Default: empty string,
which checks every BMH in the namespace.

### `root_disk_raid_expected_models`

List of model substrings (case-insensitive) that identify a RAID-backed root
disk.  Any storage entry whose model contains at least one entry from this list
is considered RAID-backed.  The list is joined into a single alternation regex
at runtime, so entries do not need to use regex syntax.

Defined in **`vars/main.yml`** (not `defaults/`), so it is the authoritative
list for the role.  Add new hardware models there as they are onboarded.

Current entries:

```yaml
root_disk_raid_expected_models:
  - "PERC"        # Dell PERC H-series controllers (e.g. "PERC H745 Frnt")
  - "RAID"        # Generic RAID virtual disk label
  - "DELLBOSS VD" # Dell BOSS (Boot Optimized Storage Solution) virtual disk
  - "Smart Array" # HPE Smart Array controllers (e.g. "Smart Array P816i-a SR Gen10")
  - "HPE"         # HPE RAID virtual disk labels (e.g. "HPE Logical Volume")
```

To add a new model, append it to `vars/main.yml`.  If you need to override the
list from inventory or a playbook without editing the role, define the variable
there and include all entries you still want matched.

### `root_disk_raid_required`

Controls whether unproven or failed checks become `FAIL` or `WARN`. Default:
`true`.

## Requirements

- `oc` CLI available on the target host with a valid kubeconfig.
- Target host: `openshift_bastion` via `readiness_validation.yml`.
- BareMetalHosts expose either `spec.rootDeviceHints.wwn` or
  `spec.rootDeviceHints.deviceName`, and `status.hardware.storage` is populated
  in `root_disk_raid_bmh_namespace`.
