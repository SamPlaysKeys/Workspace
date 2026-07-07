# Greenfield Readiness Role Examples

This directory documents one sanitized rebuild example for every readiness role wired into
`openshift-installation/playbooks/readiness_validation.yml`.

Use these examples with `../../templates/role_skeleton/` when rebuilding the readiness suite in a
new environment. The examples preserve the production role patterns while replacing organization-
specific receivers, endpoints, group prefixes, and knowledge-base references with generic values.

## Shared Contract

Every role follows the same contract:

- Reset a private result list at the start of `tasks/main.yml`.
- Query state with read-only commands using `changed_when: false` and usually `failed_when: false`.
- Convert command failures into normal `PASS`, `WARN`, or `FAIL` result entries.
- Print a human-readable debug report.
- Append one structured dict to `readiness_sections`.
- Append failures to `readiness_failures` only when local results contain `FAIL` entries.
- Increment `readiness_warn_count` by the local warning count.
- Never call `ansible.builtin.fail` inside the role.

Each role play in the parent playbook should set `environment: KUBECONFIG: "{{ ocp_kubeconfig }}"`
at play level so role tasks do not repeat kubeconfig handling.

## Role Index

| Role | Section key | Primary result list | Purpose |
|---|---|---|---|
| `readiness_check_software_versions` | `software_versions` | `_csv_results` | Validate OpenShift version, installed operator CSVs, and optional storage operator version. |
| `readiness_check_cluster_operators` | `cluster_operators` | `_co_results` | Validate ClusterOperator and MachineConfigPool health. |
| `readiness_alert_routing` | `alert_routing` | `_alert_results` | Validate Alertmanager receivers and endpoint reachability. |
| `readiness_alert_status` | `alert_status` | `_alert_report_firing`, `_alert_report_silenced` | Classify firing and silenced alerts against a known-alerts list. |
| `readiness_check_nads` | `nads` | `_nads_results` | Validate NetworkAttachmentDefinition structure and expected VLAN coverage. |
| `readiness_check_logging` | `logging` | `_logging_results` | Validate logging stack resources, storage secret shape, and tenant queries. |
| `readiness_check_monitoring_pvcs` | `monitoring_pvcs` | `_monitoring_pvc_results` | Validate expected monitoring PVCs exist and are bound. |
| `readiness_check_mhc` | `mhc` | `_mhc_results` | Validate MachineHealthChecks, with master fallback for compact clusters. |
| `readiness_check_root_disk_raid` | `root_disk_raid` | `_root_disk_raid_results` | Validate BareMetalHost root-device hints resolve to expected RAID-backed disk models. |
| `readiness_check_cpu_overcommit` | `cpu_overcommit` | `_cpu_overcommit_results` | Validate OpenShift Virtualization CPU allocation ratio for the environment tier. |
| `readiness_check_rbac` | `rbac` | `_rbac_results` | Report groups and fail when required groups are absent. |
| `readiness_check_rhacm` | `rhacm` | `_rhacm_results` | Validate managed-cluster agent namespace, Klusterlet, and agent pods. |
| `readiness_issue_remediation` | `issue_remediation` | `_remediation_results` | Match previous readiness results to remediation recommendations. |

## 1. `readiness_check_software_versions`

### Rebuild Purpose

Use this role to ensure required OpenShift operators are installed at authorized CSV versions.
The role also records unexpected installed CSVs as warnings so operators can see drift.

### Sanitized Defaults

```yaml
authorized_operators:
  cluster-logging:
    required: true
    version:
      - "cluster-logging\\.v6\\.2\\.3"
  loki-operator:
    required: true
    version:
      - "loki-operator\\.v6\\.2\\.3"
  kubernetes-nmstate-operator:
    required: true
    version:
      - "kubernetes-nmstate-operator\\.4\\.18\\.0-2025\\d+"
  packageserver:
    required: true
    version:
      - "packageserver"
  optional-storage-operator:
    required: false
    version:
      - "optional-storage-operator\\.v1\\.0\\.0"

storagecluster_version:
  - "1.0.0"
```

### Core Task Pattern

```yaml
- name: Get installed CSVs
  ansible.builtin.command:
    cmd: oc get csv --all-namespaces --output custom-columns=NAME:.metadata.name --no-headers
  register: _csv_installed_raw
  changed_when: false

- name: Build installed CSV list and reset results
  ansible.builtin.set_fact:
    _csv_installed: "{{ _csv_installed_raw.stdout_lines | select('ne', '') | list | unique | sort }}"
    _csv_results: []

- name: Check authorized operators
  vars:
    _matched: "{{ _csv_installed | select('match', '^(' ~ (item.value.version | join('|')) ~ ')$') | list }}"
    _close_match: "{{ _csv_installed | select('search', item.key) | list }}"
  ansible.builtin.set_fact:
    _csv_results: >-
      {{
        _csv_results + (
          (_matched | length > 0) | ternary(
            [{'csv': _matched | first, 'status': 'PASS', 'detail': 'matched authorized pattern', 'required': item.value.required, 'close_matches': []}],
            [{'csv': item.key, 'status': item.value.required | ternary('FAIL', 'WARN'), 'detail': 'Expected: ' ~ (item.value.version | join(' OR ')), 'required': item.value.required, 'close_matches': _close_match}]
          )
        )
      }}
  loop: "{{ authorized_operators | dict2items }}"
  loop_control:
    label: "{{ item.key }}"
```

### Rebuild Notes

- CSV regexes are Python regexes matched as full strings.
- Escape literal dots in CSV names.
- Required missing operators are `FAIL`; optional missing operators are `WARN`.
- Unexpected installed CSVs should be appended as `WARN` entries.

## 2. `readiness_check_cluster_operators`

### Rebuild Purpose

Use this role to check core OpenShift health from ClusterOperator and MachineConfigPool condition
fields.

### Core Task Pattern

```yaml
- name: Reset cluster operator results list
  ansible.builtin.set_fact:
    _co_results: []

- name: Get ClusterOperators
  ansible.builtin.command:
    cmd: oc get co -o json
  register: _co_raw
  changed_when: false
  failed_when: false

- name: Record ClusterOperator API query failure
  ansible.builtin.set_fact:
    _co_results: "{{ _co_results + [{'type': 'co', 'name': 'oc-api', 'status': 'FAIL', 'detail': 'oc get co failed rc=' ~ _co_raw.rc, 'required': true}] }}"
  when: _co_raw.rc != 0

- name: Evaluate each ClusterOperator
  vars:
    _available: "{{ item.status.conditions | selectattr('type', 'eq', 'Available') | map(attribute='status') | first | default('Unknown') }}"
    _progressing: "{{ item.status.conditions | selectattr('type', 'eq', 'Progressing') | map(attribute='status') | first | default('Unknown') }}"
    _degraded: "{{ item.status.conditions | selectattr('type', 'eq', 'Degraded') | map(attribute='status') | first | default('Unknown') }}"
  ansible.builtin.set_fact:
    _co_results: >-
      {{
        _co_results + [{
          'type': 'co',
          'name': item.metadata.name,
          'status': (_available == 'False' or _degraded == 'True') | ternary('FAIL', (_progressing == 'True') | ternary('WARN', 'PASS')),
          'detail': 'Available=' ~ _available ~ ' Progressing=' ~ _progressing ~ ' Degraded=' ~ _degraded
        }]
      }}
  loop: "{{ (_co_raw.stdout | from_json).items }}"
  loop_control:
    label: "{{ item.metadata.name }}"
  when: _co_raw.rc == 0
```

### Rebuild Notes

- Compare OpenShift condition statuses to strings: `'True'` and `'False'`.
- ClusterOperator `Available=False` or `Degraded=True` is `FAIL`.
- ClusterOperator `Progressing=True` without hard failure is `WARN`.
- MachineConfigPool `Degraded=True` is `FAIL`; `Updated=False` is `WARN`.
- Implement `check_co.yml` and `check_mcp.yml` as included task files for readability.

## 3. `readiness_alert_routing`

### Rebuild Purpose

Use this role to verify Alertmanager routing contains required receivers and can reach required
notification endpoints from inside the Alertmanager pod.

### Sanitized Defaults

```yaml
alertmanager_pod: "alertmanager-main-0"
alertmanager_namespace: "openshift-monitoring"
alertmanager_url: "http://localhost:9093"

alert_routing_required_receivers:
  - name: "critical-alerts"
    required: true
  - name: "default"
    required: true

alert_routing_endpoints:
  - url: "https://alerts.example.invalid/receiver"
    required: true
  - url: "https://hooks.example.invalid/services/alerts"
    required: false
```

### Core Task Pattern

```yaml
- name: Reset alert routing results list
  ansible.builtin.set_fact:
    _alert_results: []

- name: Get alertmanager route config via amtool
  ansible.builtin.command:
    cmd: >-
      oc exec {{ alertmanager_pod }} -c alertmanager -n {{ alertmanager_namespace }}
      -- /bin/bash -c "amtool config routes show --alertmanager.url {{ alertmanager_url }}"
  register: _amtool_raw
  changed_when: false
  failed_when: false

- name: Check each required receiver is present
  ansible.builtin.set_fact:
    _alert_results: >-
      {{
        _alert_results + [{
          'check': 'receiver: ' ~ item.name,
          'status': (_amtool_raw.rc != 0) | ternary(item.required | ternary('FAIL', 'WARN'), (item.name in _amtool_raw.stdout) | ternary('PASS', item.required | ternary('FAIL', 'WARN'))),
          'detail': (_amtool_raw.rc != 0) | ternary('amtool exec failed', (item.name in _amtool_raw.stdout) | ternary('Found in route config', 'Not found in route config'))
        }]
      }}
  loop: "{{ alert_routing_required_receivers }}"
  loop_control:
    label: "{{ item.name }}"
```

### Rebuild Notes

- Treat endpoint HTTP `200` and `301` as reachable unless the local standard is stricter.
- Endpoint failures follow each item's `required` flag.
- Keep URLs in defaults generic and environment-owned.

## 4. `readiness_alert_status`

### Rebuild Purpose

Use this role to query Alertmanager, classify firing alerts, and fail only on unrecognized firing
alerts. Known firing alerts become warnings with operator guidance.

### Sanitized Defaults

```yaml
alertmanager_pod: "alertmanager-main-0"
alertmanager_namespace: "openshift-monitoring"
alertmanager_url: "http://localhost:9093"

known_alerts:
  Watchdog:
    category: "known_silence"
    explanation: "Synthetic always-firing alert used to verify the alerting pipeline."
    remediation: "No action required when the alerting pipeline is otherwise healthy."
    reference: "https://runbooks.prometheus-operator.dev/runbooks/general/watchdog/"
  ExampleActionRequired:
    category: "known_fix"
    explanation: "Example alert with a known remediation."
    remediation: "Follow the local runbook for this alert."
    reference: ""
```

### Core Task Pattern

```yaml
- name: Query firing alerts
  ansible.builtin.command:
    cmd: >-
      oc exec {{ alertmanager_pod }} -n {{ alertmanager_namespace }} -c alertmanager
      -- amtool --alertmanager.url {{ alertmanager_url }} alert query -a --output=json
  register: _amtool_firing
  changed_when: false
  failed_when: false

- name: Parse firing alert instances
  vars:
    _raw_json: "{{ _amtool_firing.stdout if _amtool_firing.stdout | length > 0 else '[]' }}"
  ansible.builtin.set_fact:
    _firing_instances: "{{ _raw_json if (_raw_json is sequence and _raw_json is not string) else (_raw_json | from_json) }}"
  when: _amtool_firing.rc == 0

- name: Correlate unknown firing alert instances
  vars:
    _labels: "{{ item.labels | default({}) }}"
    _name: "{{ _labels.get('alertname', 'unknown') }}"
  ansible.builtin.set_fact:
    _alert_report_firing: >-
      {{
        _alert_report_firing | default([]) + [{
          'name': _name,
          'known': false,
          'category': 'unknown',
          'status': 'UNKNOWN',
          'explanation': 'No entry in known_alerts; investigate.',
          'remediation': '',
          'reference': '',
          'detail': ''
        }]
      }}
  loop: "{{ _firing_instances | default([]) }}"
  loop_control:
    label: "{{ item.labels.get('alertname', 'unknown') }}"
  when: item.labels.get('alertname', '') not in known_alerts
```

### Rebuild Notes

- Firing alert statuses are domain-specific: `UNKNOWN` is a readiness `FAIL`; `ACTION`, `MONITOR`, and `SILENCE` are warnings.
- Preserve labels for unknown alerts so engineers can identify namespace, pod, container, job, or instance.
- Keep the known-alerts list generic in examples; environment teams should own real runbook content.

## 5. `readiness_check_nads`

### Rebuild Purpose

Use this role to validate NetworkAttachmentDefinitions and optionally verify every expected VLAN has
a matching NAD.

### Sanitized Defaults

```yaml
nad_namespace: default

expected_nad_vlans:
  - vlan: 100
    required: true
  - vlan: 200
    required: false
```

### Core Task Pattern

```yaml
- name: Reset NAD results list
  ansible.builtin.set_fact:
    _nads_results: []

- name: Get all NetworkAttachmentDefinitions
  ansible.builtin.command:
    cmd: oc get net-attach-def -n {{ nad_namespace }} -o json
  register: _nads_raw
  changed_when: false
  failed_when: false

- name: Evaluate each NAD for valid VLAN configuration
  vars:
    _config: "{{ item.spec.config if (item.spec.config is mapping) else (item.spec.config | from_json) }}"
    _vlan_raw: "{{ _config.get('vlan', '') }}"
    _has_vlan: "{{ _vlan_raw | int(0) > 0 }}"
  ansible.builtin.set_fact:
    _nads_results: >-
      {{
        _nads_results + [{
          'name': item.metadata.name,
          'status': _has_vlan | ternary('PASS', 'WARN'),
          'detail': _has_vlan | ternary('VLAN: ' ~ (_vlan_raw | int), 'No numeric VLAN in spec.config'),
          'required': false,
          'section': 'inventory',
          'vlan_id': _has_vlan | ternary(_vlan_raw | int, -1)
        }]
      }}
  loop: "{{ _nads_items }}"
  loop_control:
    label: "{{ item.metadata.name }}"
```

### Rebuild Notes

- `spec.config` is a JSON string on many clusters; parse it before checking `vlan`.
- Structural NAD issues are warnings by default.
- Expected VLAN coverage can be required or optional per VLAN.

## 6. `readiness_check_logging`

### Rebuild Purpose

Use this role to validate observability logging resources and verify live tenant queries can return
recent data.

### Sanitized Defaults

```yaml
logging_namespace: openshift-logging
logging_lokistack_name: lokistack
logging_loki_s3_secret_name: logging-loki-s3

logging_tenants:
  - name: application
    required: true
  - name: audit
    required: true
  - name: infrastructure
    required: true
```

### Core Task Pattern

```yaml
- name: Reset logging results list
  ansible.builtin.set_fact:
    _logging_results: []

- name: Get LokiStack
  ansible.builtin.command:
    cmd: oc get lokistack/{{ logging_lokistack_name }} -n {{ logging_namespace }} -o json
  register: _lokistack_raw
  changed_when: false
  failed_when: false

- name: Evaluate LokiStack health
  vars:
    _found: "{{ _lokistack_raw.rc == 0 }}"
    _ls_stdout: "{{ _lokistack_raw.stdout if _found else '{}' }}"
    _ls: "{{ _ls_stdout if (_ls_stdout is mapping) else (_ls_stdout | from_json) }}"
    _ready: "{{ _ls.get('status', {}).get('conditions', []) | selectattr('type', 'eq', 'Ready') | map(attribute='status') | first | default('Unknown') }}"
  ansible.builtin.set_fact:
    _logging_results: >-
      {{
        _logging_results + [{
          'name': 'lokistack/' ~ logging_lokistack_name,
          'status': (not _found) | ternary('WARN', (_ready == 'True') | ternary('PASS', 'WARN')),
          'detail': (not _found) | ternary('Not found in ' ~ logging_namespace, 'Ready=' ~ _ready),
          'required': false,
          'section': 'infrastructure'
        }]
      }}
```

### Rebuild Notes

- Missing optional logging infrastructure is a `WARN` unless your environment requires it.
- Missing or malformed object-storage credentials should be `FAIL` when the logging stack is required.
- Put tenant query logic in an included `check_loki_tenants.yml` task file.

## 7. `readiness_check_monitoring_pvcs`

### Rebuild Purpose

Use this role to verify expected Prometheus and Alertmanager PVCs exist and are bound.

### Sanitized Defaults

```yaml
monitoring_pvcs_expected:
  - name: alertmanager-main-db-alertmanager-main-0
    namespace: openshift-monitoring
    required: true
  - name: prometheus-k8s-db-prometheus-k8s-0
    namespace: openshift-monitoring
    required: true
  - name: prometheus-user-workload-db-prometheus-user-workload-0
    namespace: openshift-user-workload-monitoring
    required: false
```

### Core Task Pattern

```yaml
- name: Reset monitoring PVC results list
  ansible.builtin.set_fact:
    _monitoring_pvc_results: []

- name: Fetch PVCs from openshift-monitoring
  ansible.builtin.command:
    cmd: oc get pvc -n openshift-monitoring -o json
  register: _core_pvc_raw
  changed_when: false
  failed_when: false

- name: Build PVC phase lookup
  vars:
    _safe: "{{ _core_pvc_raw.stdout if _core_pvc_raw.rc == 0 else '{\"items\":[]}' }}"
    _items: "{{ (_safe if (_safe is mapping) else (_safe | from_json)).get('items', []) }}"
  ansible.builtin.set_fact:
    _core_pvc_phases: "{{ dict(_items | map(attribute='metadata.name') | zip(_items | map(attribute='status.phase'))) }}"

- name: Check monitoring PVC status
  vars:
    _phase: "{{ _core_pvc_phases.get(item.name, '') }}"
    _exists: "{{ item.name in _core_pvc_phases }}"
    _status: "{{ (_exists and _phase == 'Bound') | ternary('PASS', item.required | ternary('FAIL', 'WARN')) }}"
  ansible.builtin.set_fact:
    _monitoring_pvc_results: "{{ _monitoring_pvc_results + [{'name': item.name, 'namespace': item.namespace, 'status': _status, 'detail': _exists | ternary('phase=' ~ _phase, 'PVC not found'), 'required': item.required}] }}"
  loop: "{{ monitoring_pvcs_expected }}"
  loop_control:
    label: "{{ item.namespace }}/{{ item.name }}"
```

### Rebuild Notes

- Required missing PVCs are `FAIL`.
- Existing but non-`Bound` PVCs are `WARN` in the production pattern.
- User-workload monitoring PVCs are usually optional because UWM may not be enabled everywhere.

## 8. `readiness_check_mhc`

### Rebuild Purpose

Use this role to validate MachineHealthCheck presence and status. Standard clusters check worker
MHCs; compact clusters can fall back to master MHCs when worker MHCs do not exist.

### Sanitized Defaults

```yaml
mhc_namespace: openshift-machine-api

mhc_expected:
  - name: worker-us-east-1a
    required: true
  - name: worker-us-east-1b
    required: true

mhc_master_fallback:
  - name: master
    required: true
```

### Core Task Pattern

```yaml
- name: Reset MachineHealthCheck results list
  ansible.builtin.set_fact:
    _mhc_results: []

- name: Get MachineHealthChecks
  ansible.builtin.command:
    cmd: oc get machinehealthcheck -n {{ mhc_namespace }} -o json
  register: _mhc_raw
  changed_when: false
  failed_when: false

- name: Evaluate MachineHealthCheck existence and status
  vars:
    _exists: "{{ item.name in _mhc_names }}"
    _mhc_obj: "{{ _mhc_lookup.get(item.name, {}) }}"
    _conditions: "{{ _mhc_obj.get('status', {}).get('conditions', []) }}"
    _remediation_allowed: "{{ _conditions | selectattr('type', 'eq', 'RemediationAllowed') | map(attribute='status') | first | default('Unknown') }}"
    _current_healthy: "{{ _mhc_obj.get('status', {}).get('currentHealthy', -1) | int }}"
    _expected_machines: "{{ _mhc_obj.get('status', {}).get('expectedMachines', -1) | int }}"
  ansible.builtin.set_fact:
    _mhc_results: >-
      {{
        _mhc_results + [{
          'name': 'machinehealthcheck/' ~ item.name,
          'status': (not _exists) | ternary(item.required | ternary('FAIL', 'WARN'), (_remediation_allowed == 'False') | ternary('FAIL', (_current_healthy != _expected_machines) | ternary('WARN', 'PASS'))),
          'detail': (not _exists) | ternary('Not found', 'RemediationAllowed=' ~ _remediation_allowed ~ ' currentHealthy=' ~ _current_healthy ~ '/' ~ _expected_machines),
          'required': item.required
        }]
      }}
  loop: "{{ _mhc_effective_expected }}"
  loop_control:
    label: "{{ item.name }}"
```

### Rebuild Notes

- `RemediationAllowed=False` is a hard failure.
- Unknown or mismatched health counts are warnings unless the MHC itself is missing and required.
- Keep fallback selection explicit and reported in the debug output.

## 9. `readiness_check_root_disk_raid`

### Rebuild Purpose

Use this role to validate that BareMetalHost `spec.rootDeviceHints.wwn` points to a storage device
whose model indicates RAID-backed storage.

### Sanitized Defaults

```yaml
root_disk_raid_bmh_namespace: openshift-machine-api
root_disk_raid_bmh_name_regex: ""
root_disk_raid_expected_model_regex: "(?i)(PERC|RAID)"
root_disk_raid_required: true
```

### Core Task Pattern

```yaml
- name: Reset root disk RAID results list
  ansible.builtin.set_fact:
    _root_disk_raid_results: []

- name: Get BareMetalHosts
  ansible.builtin.command:
    cmd: oc get baremetalhost -n {{ root_disk_raid_bmh_namespace }} -o json
  register: _root_disk_raid_bmh_raw
  changed_when: false
  failed_when: false

- name: Evaluate BareMetalHost root disk backing
  vars:
    _name: "{{ item.get('metadata', {}).get('name', 'unknown') }}"
    _hint_wwn: "{{ item.get('spec', {}).get('rootDeviceHints', {}).get('wwn', '') }}"
    _hint_wwn_normalized: "{{ _hint_wwn | lower | regex_replace('^0x', '') | regex_replace('[^0-9a-f]', '') }}"
    _storage: "{{ item.get('status', {}).get('hardware', {}).get('storage', []) }}"
    _matches: >-
      {%- set matches = [] -%}
      {%- for disk in _storage -%}
      {%- set disk_wwn = disk.get('wwn', '') | lower | regex_replace('^0x', '') | regex_replace('[^0-9a-f]', '') -%}
      {%- if _hint_wwn_normalized | length > 0 and disk_wwn == _hint_wwn_normalized -%}
      {%- set _ = matches.append(disk) -%}
      {%- endif -%}
      {%- endfor -%}
      {{ matches }}
    _matched_disk: "{{ _matches | first | default({}) }}"
    _passed: "{{ _matched_disk | length > 0 and (_matched_disk.get('model', '') is search(root_disk_raid_expected_model_regex)) }}"
  ansible.builtin.set_fact:
    _root_disk_raid_results: "{{ _root_disk_raid_results + [{'name': 'baremetalhost/' ~ _name, 'status': _passed | ternary('PASS', root_disk_raid_required | ternary('FAIL', 'WARN')), 'detail': 'hint=' ~ _hint_wwn ~ ' model=' ~ _matched_disk.get('model', '?'), 'required': root_disk_raid_required}] }}"
  loop: "{{ _root_disk_raid_bmh_targets }}"
  loop_control:
    label: "{{ item.get('metadata', {}).get('name', 'unknown') }}"
```

### Rebuild Notes

- Normalize WWNs before comparing: lowercase, remove leading `0x`, and remove non-hex characters.
- Do not assume BareMetalHost JSON fields are present; use `.get()` chains.
- The check proves the BMH root hint targets a RAID-like storage model, not the live OS root device.

## 10. `readiness_check_cpu_overcommit`

### Rebuild Purpose

Use this role to validate `spec.resourceRequirements.vmiCPUAllocationRatio` on the HyperConverged
custom resource against an expected ratio for the environment tier.

### Sanitized Defaults

```yaml
cpu_overcommit_namespace: openshift-cnv
cpu_overcommit_hco_name: kubevirt-hyperconverged
cpu_overcommit_fallback_tier: production

cpu_overcommit_env_classification:
  dev: nonproduction
  test: nonproduction
  stage: production
  prod: production

cpu_overcommit_expected_ratios:
  nonproduction: 10
  production: 4
```

### Core Task Pattern

```yaml
- name: Reset CPU overcommit results list
  ansible.builtin.set_fact:
    _cpu_overcommit_results: []

- name: Classify cluster env into tier
  vars:
    _env_defined: "{{ env is defined and (env | default('') | length > 0) }}"
    _env_known: "{{ _env_defined and ((env | default('')) in cpu_overcommit_env_classification) }}"
  ansible.builtin.set_fact:
    _cpu_overcommit_tier: "{{ _env_known | ternary(cpu_overcommit_env_classification[env | default('')], cpu_overcommit_fallback_tier) }}"

- name: Get HyperConverged CR
  ansible.builtin.command:
    cmd: oc -n {{ cpu_overcommit_namespace }} get hyperconverged {{ cpu_overcommit_hco_name }} -o json
  register: _cpu_overcommit_hco_raw
  changed_when: false
  failed_when: false

- name: Evaluate vmiCPUAllocationRatio
  vars:
    _safe: "{{ _cpu_overcommit_hco_raw.stdout if _cpu_overcommit_hco_raw.rc == 0 else '{}' }}"
    _parsed: "{{ _safe if (_safe is mapping) else (_safe | from_json) }}"
    _observed: "{{ _parsed.get('spec', {}).get('resourceRequirements', {}).get('vmiCPUAllocationRatio', None) }}"
    _expected: "{{ cpu_overcommit_expected_ratios[_cpu_overcommit_tier] }}"
  ansible.builtin.set_fact:
    _cpu_overcommit_results: "{{ _cpu_overcommit_results + [{'name': 'hyperconverged/' ~ cpu_overcommit_hco_name ~ '.vmiCPUAllocationRatio', 'status': (_cpu_overcommit_hco_raw.rc != 0) | ternary('WARN', (_observed is none) | ternary('FAIL', ((_observed | int) == (_expected | int)) | ternary('PASS', 'FAIL'))), 'detail': 'observed=' ~ (_observed | default('missing', true)) ~ ' expected=' ~ _expected ~ ' tier=' ~ _cpu_overcommit_tier}] }}"
```

### Rebuild Notes

- Missing OpenShift Virtualization is a `WARN` because the ratio is not meaningful without HCO.
- Missing ratio field or wrong ratio is `FAIL` when HCO exists.
- Unknown `env` should produce a warning and use the fallback tier.

## 11. `readiness_check_rbac`

### Rebuild Purpose

Use this role to report OpenShift groups and fail if required groups are missing.

### Sanitized Defaults

```yaml
rbac_required_groups:
  - cluster-admins
  - platform-operators

rbac_collapsed_group_prefix_regex: "^team-k8s-"
rbac_collapsed_group_label: "team-k8s-[...] Groups"
```

### Core Task Pattern

```yaml
- name: Get OpenShift Groups
  ansible.builtin.command:
    cmd: oc get groups -o json
  register: _rbac_groups_raw
  changed_when: false
  failed_when: false

- name: Parse Group list
  vars:
    _safe: "{{ _rbac_groups_raw.stdout if _rbac_groups_raw.rc == 0 else '{\"items\":[]}' }}"
    _all_items: "{{ (_safe if (_safe is mapping) else (_safe | from_json)).get('items', []) }}"
  ansible.builtin.set_fact:
    _rbac_items: "{{ _all_items | rejectattr('metadata.name', 'match', rbac_collapsed_group_prefix_regex) | list }}"
    _rbac_collapsed_count: "{{ _all_items | selectattr('metadata.name', 'match', rbac_collapsed_group_prefix_regex) | list | length }}"
    _rbac_results: []

- name: Flag missing required groups
  ansible.builtin.set_fact:
    _rbac_results: "{{ _rbac_results + [{'name': item, 'user_count': -1, 'created': 'n/a', 'status': 'MISSING'}] }}"
  loop: "{{ rbac_required_groups }}"
  loop_control:
    label: "{{ item }}"
  when: item not in (_rbac_items | map(attribute='metadata.name') | list)
```

### Rebuild Notes

- Make collapsed group prefix configurable. Do not hard-code organization naming conventions in reusable examples.
- `MISSING` required groups map to section `fail` count.
- Present required groups can use `REQUIRED`; non-required groups can use `PRESENT`.

## 12. `readiness_check_rhacm`

### Rebuild Purpose

Use this role to validate that a cluster has been imported into Advanced Cluster Management and that
its agent components are healthy enough to communicate.

### Sanitized Defaults

```yaml
rhacm_agent_namespace: open-cluster-management-agent
rhacm_addon_namespace: open-cluster-management-agent-addon
rhacm_klusterlet_name: klusterlet
rhacm_check_required: false
```

### Core Task Pattern

```yaml
- name: Reset RHACM results list
  ansible.builtin.set_fact:
    _rhacm_results: []

- name: Check agent namespace
  ansible.builtin.command:
    cmd: oc get namespace {{ rhacm_agent_namespace }} -o jsonpath='{.status.phase}'
  register: _rhacm_agent_ns_raw
  changed_when: false
  failed_when: false

- name: Evaluate agent namespace existence
  vars:
    _found: "{{ _rhacm_agent_ns_raw.rc == 0 and _rhacm_agent_ns_raw.stdout | trim == 'Active' }}"
  ansible.builtin.set_fact:
    _rhacm_results: "{{ _rhacm_results + [{'name': 'namespace/' ~ rhacm_agent_namespace, 'status': _found | ternary('PASS', 'WARN'), 'detail': _found | ternary('Present and Active', 'Not found; cluster may not be imported')}] }}"

- name: Evaluate Klusterlet Available condition
  vars:
    _conditions: "{{ _rhacm_klusterlet.get('status', {}).get('conditions', []) }}"
    _available_status: "{{ _conditions | selectattr('type', 'eq', 'Available') | map(attribute='status') | first | default('Unknown') }}"
  ansible.builtin.set_fact:
    _rhacm_results: "{{ _rhacm_results + [{'name': 'klusterlet/' ~ rhacm_klusterlet_name, 'status': (_available_status == 'True') | ternary('PASS', 'WARN'), 'detail': 'Available=' ~ _available_status}] }}"
```

### Rebuild Notes

- RHACM checks are warnings unless `rhacm_check_required: true`.
- Missing agent namespace usually means import has not completed.
- Agent pod checks should require `phase=Running` and Ready condition `'True'`.

## 13. `readiness_issue_remediation`

### Rebuild Purpose

Use this advisory role to read previous `readiness_sections` and map known failure signatures to
operator remediation notes. This role does not query the cluster and does not write to
`readiness_failures`.

### Sanitized Defaults

```yaml
remediation_knowledge_base:
  software_versions_mismatch:
    issue: "Operator version mismatch"
    root_cause: "One or more installed CSVs does not match the authorized version list."
    resolution: "Review the operator subscription, channel, install plan, and authorized CSV baseline."
    reference: ""
    doc: "software_versions_mismatch.md"
  mhc_worker:
    issue: "MachineHealthCheck degraded"
    root_cause: "One or more expected MHC resources is missing or not permitting remediation."
    resolution: "Inspect MachineHealthCheck status and worker machine health."
    reference: ""
    doc: "mhc_worker.md"
```

### Core Task Pattern

```yaml
- name: Reset issue remediation results list
  ansible.builtin.set_fact:
    _remediation_results: []

- name: Extract software_versions section
  ansible.builtin.set_fact:
    _sec_software_versions: "{{ readiness_sections | selectattr('section', 'eq', 'software_versions') | list | first | default({}) }}"

- name: Match software version failures against knowledge base
  vars:
    _kb_entry: "{{ remediation_knowledge_base['software_versions_mismatch'] | default({}) }}"
    _failed_csvs: "{{ (_sec_software_versions.results | default([])) | selectattr('status', 'eq', 'FAIL') | map(attribute='csv') | list }}"
  ansible.builtin.set_fact:
    _remediation_results: >-
      {{
        _remediation_results + ([{
          'issue': _kb_entry.issue,
          'source': 'software_versions',
          'trigger': (_failed_csvs | length | string) ~ ' CSV(s) failed: ' ~ (_failed_csvs | join(', ')),
          'root_cause': _kb_entry.root_cause,
          'resolution': _kb_entry.resolution,
          'reference': _kb_entry.reference,
          'doc': _kb_entry.doc,
          'kb_key': 'software_versions_mismatch'
        }] if _kb_entry | length > 0 and _failed_csvs | length > 0 else [])
      }}
  when: _sec_software_versions | length > 0
```

### Rebuild Notes

- This role should append an `issue_remediation` section with warning results only.
- Deduplicate results by `kb_key` before reporting.
- Do not add remediation recommendations to `readiness_failures`; they are advisory evidence attached to earlier failures.

## Standard Section Append

Every rebuild should finish each role with this shape, replacing names and result variables:

```yaml
- name: Append role section to readiness_sections
  vars:
    _pass: "{{ _role_results | selectattr('status', 'eq', 'PASS') | list | length }}"
    _warn: "{{ _role_results | selectattr('status', 'eq', 'WARN') | list | length }}"
    _fail: "{{ _role_results | selectattr('status', 'eq', 'FAIL') | list | length }}"
  ansible.builtin.set_fact:
    readiness_sections: >-
      {{
        readiness_sections + [{
          'section': 'role_section_key',
          'pass': _pass | int,
          'warn': _warn | int,
          'fail': _fail | int,
          'results': _role_results
        }]
      }}
```

Roles that use custom statuses, such as `readiness_alert_status`, `readiness_check_rbac`, and
`readiness_issue_remediation`, should map those custom statuses into `pass`, `warn`, and `fail`
counts in the section append task.
