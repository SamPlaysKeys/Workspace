# Readiness Check Role Authoring Guide

> **Scope**: This guide is for AI agents and human authors writing new Ansible readiness-check
> roles that plug into the multi-play validation playbook pattern established in this repository.
> It is derived from real bugs encountered and fixed during development and contains no
> client-specific information.

---

## Table of Contents

1. [What is a readiness check role?](#1-what-is-a-readiness-check-role)
2. [Playbook architecture](#2-playbook-architecture)
3. [Role anatomy](#3-role-anatomy)
4. [Naming and tagging conventions](#4-naming-and-tagging-conventions)
5. [The shared-state contract](#5-the-shared-state-contract)
6. [The result-accumulation pattern](#6-the-result-accumulation-pattern)
7. [Three-tier status codes: PASS / WARN / FAIL](#7-three-tier-status-codes-pass--warn--fail)
8. [Structured section reporting](#8-structured-section-reporting)
9. [Critical Ansible gotchas](#9-critical-ansible-gotchas)
10. [Reading OpenShift API condition fields](#10-reading-openshift-api-condition-fields)
11. [CSV version matching rules](#11-csv-version-matching-rules)
12. [KUBECONFIG management](#12-kubeconfig-management)
13. [Command task hygiene](#13-command-task-hygiene)
14. [Dictionary access safety](#14-dictionary-access-safety)
15. [Loop and output hygiene](#15-loop-and-output-hygiene)
16. [Failure deferral — never fail inside a role](#16-failure-deferral--never-fail-inside-a-role)
17. [Examples from existing roles](#17-examples-from-existing-roles)
18. [File checklist for a new role](#18-file-checklist-for-a-new-role)
19. [Quick reference: common mistakes](#19-quick-reference-common-mistakes)

---

## 1. What is a readiness check role?

A readiness check role is an Ansible role that:

- Queries a live OpenShift cluster (or related infrastructure) to evaluate some aspect of
  cluster health or configuration
- Produces a structured result list with `PASS`, `WARN`, or `FAIL` entries
- Appends a structured section dict to the shared `readiness_sections` fact
- Records any failures in a shared `readiness_failures` list (but **never calls `fail` itself**)
- Is invoked from a multi-play parent playbook that runs a final gate play at the end

The role itself is a pure unit of checking — not of failing. Failure is centralised.

---

## 2. Playbook architecture

The parent playbook uses a **multi-play structure**:

```
Play 1 — Bootstrap  : add bastion to dynamic group
Play 2 — Initialize : set shared facts (kubeconfig path, empty readiness_failures/sections/warn_count)
Play N — Role play  : gather_facts: false, environment: KUBECONFIG, one role
...
Play N+1 — Final gate: publish structured report via set_stats, fail if readiness_failures
```

### Why this structure?

- **Deferred failure**: All roles run even if one fails. The final gate collects all failures
  into one clean summary — not 12 separate task failures mid-play.
- **Play-level environment**: Setting `environment: KUBECONFIG` at the play level means every
  `oc` command in every task inside the role automatically inherits it. Zero per-task repetition.
- **Shared facts**: `readiness_failures`, `readiness_sections`, and `readiness_warn_count` are
  host facts set in Play 2 and mutated by each role play. Because they are host facts (not
  inventory vars), they persist across play boundaries on the same host.
- **Isolation**: Each role play has `gather_facts: false`, so there is no per-role setup overhead.

### Template: parent playbook skeleton

See `templates/parent_playbook.yml`.

### Greenfield minimum runnable environment

The role examples are enough to rebuild the checks, but a fresh environment also needs a small
Ansible/OpenShift execution scaffold around them. At minimum, build this layout:

```text
playbooks/
├── readiness_validation.yml
└── roles/
    ├── readiness_check_<role_1>/
    ├── readiness_check_<role_2>/
    └── readiness_guidelines/
```

The parent playbook must provide the same execution contract as `templates/parent_playbook.yml`:

- Add the target bastion host to an `openshift_bastion` group.
- Initialize `ocp_kubeconfig`, `readiness_failures`, `readiness_sections`, and
  `readiness_warn_count` before any role runs.
- Collect cluster identity values if the final report should include API URL, console URL, and
  cluster ID.
- Run one play per readiness role with `hosts: openshift_bastion`, `gather_facts: false`, and
  play-level `environment: KUBECONFIG: "{{ ocp_kubeconfig }}"`.
- Publish the final `readiness_report` with `ansible.builtin.set_stats`, then fail only in the
  final gate if `readiness_failures` is non-empty.

#### Control host requirements

Run the playbook from an Ansible control host that has:

- Ansible Core 2.14 or newer.
- SSH access to the bastion as `ansible_user`.
- Python on the bastion for normal Ansible module execution.
- The `oc` CLI on the bastion and in the bastion user's `PATH`.
- `jq` only if a rebuilt role explicitly shells out to it. The examples avoid requiring it by
  parsing JSON with Ansible filters.

#### Required inventory or extra vars

A minimal greenfield inventory can be as small as localhost plus runtime variables, because the
first play dynamically adds the bastion host:

```ini
[localhost]
localhost ansible_connection=local
```

Provide these variables through inventory, group vars, a job template, or `--extra-vars`:

```yaml
openshift_cluster_name: example-cluster
openshift_bastion_host_fqdn: bastion.example.internal
ansible_user: ansible
```

By default the template derives the kubeconfig path from those values:

```yaml
ocp_kubeconfig: "/home/{{ ansible_user | lower }}/{{ openshift_cluster_name }}/auth/kubeconfig"
```

If the kubeconfig lives elsewhere in a greenfield environment, either change the initialize play or
set `ocp_kubeconfig` directly there. Do not make individual roles guess kubeconfig locations.

#### Cluster access requirements

The kubeconfig user must be able to run read-only `oc get`, `oc exec`, and `oc auth can-i` style
queries for the resources checked by the enabled roles. The full suite commonly needs read access
to these API areas:

- Cluster identity resources: `Infrastructure`, `ClusterVersion`, and `Console`.
- Cluster health resources: `ClusterOperator` and `MachineConfigPool`.
- Operator inventory: `ClusterServiceVersion` across all namespaces.
- Monitoring and alerting: pods and Alertmanager resources in `openshift-monitoring`.
- Networking: `NetworkAttachmentDefinition` resources in the configured namespace.
- Logging: logging stack resources, pods, secrets, and tenant query endpoints if that role is used.
- Storage: monitoring PVCs and any optional logging/storage resources enabled in defaults.
- Bare metal: `BareMetalHost` resources for root-disk RAID validation.
- Machine API: `MachineHealthCheck` and related machine resources.
- Virtualization: `HyperConverged` when CPU overcommit validation is enabled.
- RBAC: groups and authorization checks for required access groups.
- RHACM: managed-cluster agent namespace, Klusterlet, and agent pods when RHACM validation is used.

Keep role defaults environment-owned. In a greenfield build, replace example receiver names,
endpoint URLs, VLAN IDs, operator CSV regexes, group names, namespace names, and required/optional
flags with values that match the target platform.

#### Optional component handling

Not every cluster has every component. Encode that in defaults rather than deleting checks:

- Use `required: false` for optional operators, receivers, endpoints, PVCs, and alerts.
- Use role-specific required flags such as `rhacm_check_required` or
  `root_disk_raid_required` where available.
- Missing optional infrastructure should normally produce `WARN` so operators can see that the
  check was skipped or not applicable.
- Bad configuration for infrastructure that is present and required should produce `FAIL`.

#### First-run smoke test

After rebuilding the parent playbook and at least one role, verify the scaffold before enabling the
whole suite:

```bash
ansible-playbook playbooks/readiness_validation.yml -i inventory.ini --syntax-check
ansible-playbook playbooks/readiness_validation.yml -i inventory.ini --tags cluster-operators
ansible-playbook playbooks/readiness_validation.yml -i inventory.ini --tags readiness-validation
```

For AAP or another automation controller, confirm the job artifact contains `readiness_report` with
a `summary` dict and a `sections` list. For local CLI runs, confirm each role prints its debug
status section and the final gate is the only task that can fail the playbook.

---

## 3. Role anatomy

Every readiness check role must contain exactly these files:

```
roles/readiness_check_<check_name>/
├── defaults/
│   └── main.yml     # All role-specific configuration. This is the operator baseline.
├── meta/
│   └── main.yml     # Galaxy metadata (author, description, min_ansible_version, dependencies: [])
├── tasks/
│   └── main.yml     # Entrypoint. May include sub-task files for complex roles.
└── README.md        # Documents every variable, every status code, and requirements.
```

Sub-task files (e.g. `tasks/check_something_specific.yml`) are fine for complex roles.
Always include them via `ansible.builtin.include_tasks` from `tasks/main.yml`.

---

## 4. Naming and tagging conventions

### Role name

All readiness roles are prefixed `readiness_`. New validation checks should normally use
`readiness_check_`:

```
readiness_check_<check_name>
```

Examples from this repository include `readiness_check_cluster_operators`,
`readiness_check_cpu_overcommit`, and `readiness_check_root_disk_raid`.
Existing non-check readiness roles such as `readiness_alert_routing` and
`readiness_issue_remediation` still participate in the same reporting contract.

### Internal variable prefix

All role-internal facts use a leading underscore:

```
_co_results       # local result list
_csv_installed    # parsed intermediate
_amtool_raw       # raw command output
```

This avoids polluting the host's fact namespace and makes it immediately obvious which
variables are role-private.

### Play tags

Every play in the parent playbook has **two tags**:

1. The global tag shared by all plays (`readiness-validation`)
2. A play-specific tag matching the check category (`cluster-operators`, `software-versions`, etc.)

```yaml
tags:
  - readiness-validation
  - cluster-operators
```

This lets operators run the full suite or target individual checks with `--tags`.

---

## 5. The shared-state contract

These facts are set in the **initialize play** and consumed/extended by every role:

| Fact | Type | Set by | Extended by |
|------|------|--------|-------------|
| `readiness_failures` | list of strings | initialize play | each role (append on failure) |
| `readiness_sections` | list of dicts | initialize play | each role (append one section dict) |
| `readiness_warn_count` | integer | initialize play | each role (increment by local warn count) |

A fourth fact (`ocp_kubeconfig`) is also set in the initialize play and used at the play level:

```yaml
ocp_kubeconfig: "/home/{{ ansible_user | lower }}/{{ openshift_cluster_name }}/auth/kubeconfig"
```

The current parent playbook also has a cluster-identity play that collects API URL, console URL,
and cluster ID before role execution. The final gate play publishes a single `readiness_report`
key via `set_stats` containing:

```yaml
readiness_report:
  summary:
    cluster: <cluster_name>
    cluster_id: <cluster_id>
    bastion: <bastion_fqdn>
    api_url: <api_url>
    console_url: <console_url>
    generated: <timestamp>
    result: PASS | FAIL
    failures: <int>
    warnings: <int>
    warnings_note: "<N> warning(s) recorded..." | "No warnings."
    failure_detail: <list of failure strings>
  sections:          # list in play execution order
    - section: <section_key>
      pass: <int>
      warn: <int>
      fail: <int>
      results: [...]
```

**Rules:**
- Roles read `readiness_failures | default([])` — never assume the fact exists
- Roles only **append** to `readiness_failures` — never replace it
- Roles only **append** to `readiness_sections` — never replace it
- Roles only **increment** `readiness_warn_count` — never replace it
- Roles never touch `ocp_kubeconfig`

---

## 6. The result-accumulation pattern

Every role builds an internal result list using this pattern:

### Step 1: Reset at the top of main.yml

```yaml
- name: Reset <check> results list
  ansible.builtin.set_fact:
    _my_results: []
```

This is mandatory. Without the reset, re-running the playbook or running with `--limit`
will carry stale results from the previous run into the current one.

### Step 2: Accumulate via set_fact in a loop

```yaml
- name: Evaluate each item
  ansible.builtin.set_fact:
    _my_results: >-
      {{
        _my_results + [{
          'name': item.metadata.name,
          'status': 'PASS',
          'detail': 'some detail'
        }]
      }}
  loop: "{{ _my_items }}"
  loop_control:
    label: "{{ item.metadata.name }}"
```

### Step 3: Append to shared report and record failures

See sections 8 and 16 below.

---

## 7. Three-tier status codes: PASS / WARN / FAIL

Every result entry must have a `status` field with exactly one of these values:

| Status | Meaning | Effect |
|--------|---------|--------|
| `PASS` | Check succeeded | No action required |
| `WARN` | Non-critical issue (optional check failed, or informational) | Logged but does not fail the playbook |
| `FAIL` | Required check failed | Appended to `readiness_failures`; playbook fails at gate |

### The `required` field

Each check item should carry a `required` boolean that drives the `PASS`/`WARN`/`FAIL` decision:

```yaml
'status': item.required | ternary('FAIL', 'WARN')   # when the check did not pass
```

This gives role authors a simple on/off switch in `defaults/main.yml` without changing task logic.

### Using ternary chains

For multi-condition statuses:

```yaml
'status': (
  (_degraded == true)
  | ternary('FAIL',
    (_updated == false)
    | ternary('WARN', 'PASS')
  )
)
```

Write multi-branch ternary chains as nested expressions, not as a flat if/elif. This reads left
to right: "if degraded → FAIL, else if not updated → WARN, else PASS."

---

## 8. Structured section reporting

Every role produces **two reports**:

### 8a. Console debug block

Use `ansible.builtin.debug: msg:` with a Jinja2 multiline template. Format:

```
=== Check Name Report ===

--- Section ---
STATUS name
       detail line
...

=== Summary ===
PASS : N
WARN : N
FAIL : N
```

Use `{{ '%-6s' | format(r.status) }}` to left-pad the status to 6 characters for alignment.

### 8b. Structured section appended to readiness_sections

Roles do **not** produce Markdown. Instead, they append a structured dict to `readiness_sections`
and increment `readiness_warn_count`. The final gate publishes the whole structure via `set_stats`.

```yaml
- name: Append <check> section to readiness_sections
  vars:
    _pass: "{{ _my_results | selectattr('status', 'eq', 'PASS') | list | length }}"
    _warn: "{{ _my_results | selectattr('status', 'eq', 'WARN') | list | length }}"
    _fail: "{{ _my_results | selectattr('status', 'eq', 'FAIL') | list | length }}"
  ansible.builtin.set_fact:
    readiness_sections: >-
      {{
        readiness_sections + [{
          'section': '<section_key>',
          'pass': _pass | int,
          'warn': _warn | int,
          'fail': _fail | int,
          'results': _my_results
        }]
      }}

- name: Update shared warn count
  ansible.builtin.set_fact:
    readiness_warn_count: >-
      {{ (readiness_warn_count | default(0) | int) + (_my_results | selectattr('status', 'eq', 'WARN') | list | length) }}
  when: _my_results | selectattr('status', 'eq', 'WARN') | list | length > 0
```

The `section` key must be a stable snake_case identifier (e.g. `cluster_operators`, `monitoring_pvcs`).
The `results` list contains the full result dicts (name, status, detail, and any role-specific fields).

---

## 9. Critical Ansible gotchas

Every new role author should follow these patterns because they match the readiness roles in
this repository.

### 9a. OCP API condition status values are strings

**The problem**: OpenShift API condition status fields return the strings `"True"` and `"False"`.
The readiness roles in this repository parse `oc ... -o json` output and compare condition
status values as strings.

**Consequence**: Comparisons must be consistent. Do not compare condition values to YAML
booleans unless you explicitly converted them first.

**The fix**:

```yaml
# WRONG — condition values are strings in the readiness roles:
status: (_available == true) | ternary('PASS', 'FAIL')

# CORRECT — compare condition values to API strings:
status: (_available == 'True') | ternary('PASS', 'FAIL')
```

```yaml
# WRONG — `+` is brittle when a value is not already a string:
detail: 'Available=' + _available

# CORRECT — use ~ (tilde) for string concatenation:
detail: 'Available=' ~ _available
```

### 9b. Always use `~` for string concatenation in Jinja2 / set_fact

`+` is Python's concatenation operator and only works when both operands are already strings.
If an operand is not already a string, `+` raises a `TypeError`.

`~` (tilde) in Jinja2 converts both operands to strings before joining. It is always safe.

```yaml
# WRONG:
detail: 'Available=' + _available + '  Degraded=' + _degraded

# CORRECT:
detail: 'Available=' ~ _available ~ '  Degraded=' ~ _degraded
```

### 9c. Dict attribute access vs `.get()`

**The problem**: Jinja2 dot-notation (`dict.key`) on a plain Python dict raises an
`AttributeError` before `| default()` can intercept it. This is a Jinja2/Ansible environment
quirk — `| default()` only catches `undefined` (Jinja2 Undefined objects), not Python exceptions.

**The fix**: Use Python's `.get('key', fallback)` method:

```yaml
# WRONG — AttributeError is raised before | default() fires:
explanation: known_alerts[item].explanation | default('unknown')

# CORRECT — .get() never raises, returns the fallback directly:
explanation: known_alerts.get(item, {}).get('explanation', 'unknown')
```

This pattern is required any time you access a dict that might not have the key, especially
when iterating over a list and cross-referencing against a lookup dict in `defaults/`.

### 9d. `| default()` on potentially-undefined lists before accumulation

When a `set_fact` accumulates into a list across a loop, the list fact may not exist on the
first iteration if the loop variable has not been initialized above:

```yaml
# Safe pattern — always seed with default([]):
_my_results: >-
  {{
    (_my_results | default([])) + [{ ... }]
  }}
```

The explicit `_my_results: []` reset at the top of `main.yml` makes this redundant for the
primary results list — but it is still required for secondary lists built inside conditional
branches or in sub-task files where the reset may not have run.

---

## 10. Reading OpenShift API condition fields

OpenShift resources use a standard `status.conditions` array:

```json
{
  "status": {
    "conditions": [
      { "type": "Available",    "status": "True",  "message": "..." },
      { "type": "Progressing",  "status": "False", "message": "..." },
      { "type": "Degraded",     "status": "False", "message": "..." }
    ]
  }
}
```

Extract a single condition value safely:

```yaml
vars:
  _available: >-
    {{ item.status.conditions
       | selectattr('type', 'eq', 'Available')
       | map(attribute='status')
       | first
       | default('Unknown') }}
```

Then compare:

```yaml
# CORRECT — compare condition values to API strings:
status: (_available == 'False' or _degraded == 'True') | ternary('FAIL', 'PASS')
```

**Common condition sets by resource type:**

| Resource | Condition types | PASS = |
|----------|----------------|--------|
| ClusterOperator | Available, Progressing, Degraded | Available=true, Progressing=false, Degraded=false |
| MachineConfigPool | Updated, Updating, Degraded | Updated=true, Updating=false, Degraded=false |
| Node | Ready, MemoryPressure, DiskPressure, PIDPressure | Ready=true, all others false |
| Deployment / ReplicaSet | Available, Progressing | Available=true, Progressing=false |

---

## 11. CSV version matching rules

ClusterServiceVersion (CSV) names are the canonical identifier for an installed operator plus
its version. They follow this pattern:

```
<operator-name>.<version>
```

Examples:
- `cluster-logging.v6.2.3`
- `kubernetes-nmstate-operator.4.18.0-202504080858`
- `packageserver`  (no version suffix — OLM internal)

### Matching strategy

Use `select('match', '^<pattern>$')` — anchored Python regex:

```yaml
_matched: "{{ _csv_installed | select('match', '^' + item.value.version + '$') | list }}"
```

### Regex rules for version strings

1. **Escape all literal dots**: `.` in regex means "any character"; use `\.` for literal dot.
   In YAML double-quotes, `\\.` produces the string `\.`:

   ```yaml
   version: "cluster-logging\\.v6\\.2\\.3"
   ```

2. **Variable build suffixes**: Some operators (e.g. nmstate) append a datestamp that changes
   each build. Use `\d+` to match one or more digits:

   ```yaml
   version: "kubernetes-nmstate-operator\\.4\\.18\\.0-2025\\d+"
   ```

3. **No-version operators**: Use the bare name (e.g. `packageserver`) — it matches exactly.

4. **Human-readable label** (the dict key) is for display only — it does NOT affect matching.
   The match is driven entirely by the `version` regex field.

### Detecting version mismatches vs absent operators

When an authorized operator is not matched, also check for "close matches" (same operator
name prefix, different version). Include them in the FAIL/WARN detail so the user can see
"Expected v6.2.3, found v6.3.0" rather than "not installed":

```yaml
_close_match: "{{ _csv_installed | select('search', item.key) | list }}"
```

---

## 12. KUBECONFIG management

### Set once, use everywhere

Set the kubeconfig path as a **host fact** in the initialize play, then reference it at the
**play level** in the parent playbook. Do not repeat it per task.

**In the initialize play:**

```yaml
- name: Initialize shared facts
  ansible.builtin.set_fact:
    ocp_kubeconfig: "/home/{{ ansible_user | lower }}/{{ cluster_name }}/auth/kubeconfig"
    readiness_failures: []
    readiness_sections: []
    readiness_warn_count: 0
```

**In each role play:**

```yaml
- name: Readiness validation — my check
  hosts: openshift_bastion
  gather_facts: false
  environment:
    KUBECONFIG: "{{ ocp_kubeconfig }}"
  roles:
    - my_readiness_role
```

**In role tasks: nothing.** No `environment:` blocks on individual tasks. No per-task
`KUBECONFIG=...` prefixes. The play-level environment propagates to all command tasks.

### Why play-level and not inventory var?

The kubeconfig path is constructed from runtime facts (`ansible_user`, `cluster_name`) that
may not be known at inventory parse time. Setting it as a host fact in the first play is the
cleanest way to derive it once and share it across all subsequent plays.

---

## 13. Command task hygiene

All tasks that query cluster state without changing anything must declare:

```yaml
changed_when: false
```

This prevents every `oc get ...` from being reported as a change in the play summary.

Register names for raw command output use the suffix `_raw`:

```yaml
register: _co_raw
register: _amtool_firing
register: _version_raw
```

Parsed/processed facts drop the `_raw` suffix:

```yaml
_co_items: "{{ (_co_raw.stdout | from_json)['items'] }}"
_csv_installed: "{{ _csv_installed_raw.stdout_lines | select('ne', '') | unique | sort | list }}"
```

---

## 14. Dictionary access safety

When cross-referencing items against a lookup dict (e.g. a knowledge base in `defaults/`),
always use chained `.get()` calls:

```yaml
# SAFE — returns fallback if outer or inner key is missing:
explanation: "{{ known_alerts.get(item, {}).get('explanation', 'Not in knowledge base.') }}"

# UNSAFE — raises AttributeError before | default() can fire:
explanation: "{{ known_alerts[item].explanation | default('Not in knowledge base.') }}"

# ALSO UNSAFE — dot notation raises AttributeError:
explanation: "{{ known_alerts[item] | default({}) }}.explanation"
```

The `.get(key, {}).get(key, default)` chain is the idiomatic safe pattern for
all nested dict lookups on facts derived from external data (JSON API responses, vars files, etc.).

---

## 15. Loop and output hygiene

### Always set loop_control label

```yaml
loop_control:
  label: "{{ item.metadata.name }}"
```

Without this, Ansible prints the entire loop item as the task label. For complex API objects
this produces unreadable 50-line log entries.

### Parsing stdout to lists

Strip headers and empty lines when parsing tabular command output:

```yaml
# Skip header line (index 0), reject empty lines, extract first column:
_names: >-
  {{
    _raw.stdout_lines[1:]
    | select('ne', '')
    | map('split')
    | map('first')
    | list
    | unique
    | sort
  }}
```

### Deduplication

Always deduplicate CSVs and similar lists with `| unique` — OLM reports the same CSV once
per namespace, so a role installed in 5 namespaces would appear 5 times in raw output.

---

## 16. Failure deferral — never fail inside a role

Roles **never** call `ansible.builtin.fail`. They append to the shared `readiness_failures`
list and the final gate play does the failing:

```yaml
# CORRECT — append to failures list; do not fail:
- name: Record failures
  ansible.builtin.set_fact:
    readiness_failures: >-
      {{
        (readiness_failures | default([])) +
        ['my_role: N required check(s) failed — see My Check Report above']
      }}
  when: _my_results | selectattr('status', 'eq', 'FAIL') | list | length > 0
```

**Why?** If a role fails mid-play, all subsequent roles are skipped — the user gets a partial
picture. Deferring failures lets every role run, every check produce its report, and the
final summary show all failures at once.

### Failure message format

```
<role_name>: <N> required check(s) failed — see <Report Title> above
```

The user needs to know:
1. Which role failed
2. How many checks failed
3. Where to look in the console output

---

## 17. Examples from existing roles

These examples are taken from readiness roles that exist in this repository. Prefer these
patterns over older hypothetical examples.

For a role-by-role greenfield rebuild reference, see
`examples/readiness_validation_roles/README.md`. That catalog covers every readiness role wired
into `readiness_validation.yml` with sanitized defaults, core task patterns, status rules, section
keys, and rebuild notes.

### 17a. API query failure as a result entry

`readiness_check_cluster_operators` and `readiness_check_root_disk_raid` use `failed_when: false`
for `oc` reads that may fail, then convert the failure into a normal readiness result. This lets
later checks still run and lets the final gate report the failure centrally.

```yaml
- name: Get ClusterOperators
  ansible.builtin.command:
    cmd: oc get co -o json
  register: _co_raw
  changed_when: false
  failed_when: false

- name: Record ClusterOperator API query failure
  ansible.builtin.set_fact:
    _co_results: >-
      {{
        _co_results + [{
          'type': 'co',
          'name': 'oc-api',
          'status': 'FAIL',
          'detail': 'oc get co failed (rc=' ~ _co_raw.rc ~ ') - cluster API may be temporarily unreachable',
          'required': true
        }]
      }}
  when: _co_raw.rc != 0
```

### 17b. Condition evaluation with string comparisons

`readiness_check_cluster_operators` extracts condition statuses from `status.conditions` and
compares them to the API strings `"True"` and `"False"`.

```yaml
vars:
  _available: >-
    {{ item.status.conditions
       | selectattr('type', 'eq', 'Available')
       | map(attribute='status') | first | default('Unknown') }}
  _progressing: >-
    {{ item.status.conditions
       | selectattr('type', 'eq', 'Progressing')
       | map(attribute='status') | first | default('Unknown') }}
  _degraded: >-
    {{ item.status.conditions
       | selectattr('type', 'eq', 'Degraded')
       | map(attribute='status') | first | default('Unknown') }}
status: >-
  {{
    (_available == 'False' or _degraded == 'True')
    | ternary('FAIL',
      (_progressing == 'True')
      | ternary('WARN', 'PASS')
    )
  }}
```

### 17c. Optional infrastructure as WARN, bad configuration as FAIL

`readiness_check_cpu_overcommit` records missing OpenShift Virtualization as `WARN`, because the
check is not meaningful without that component, but records a missing or wrong ratio as `FAIL`
when the component exists.

```yaml
_status: >-
  {{
    (not _cpu_overcommit_hco_present) | ternary('WARN',
      _missing_field | ternary('FAIL',
        _matches | ternary('PASS', 'FAIL')
      )
    )
  }}
```

### 17d. External JSON with safe `.get()` chains

`readiness_check_root_disk_raid` reads BareMetalHost data where nested fields may be absent.
Use `.get()` chains for this kind of external API data.

```yaml
vars:
  _name: "{{ item.get('metadata', {}).get('name', 'unknown') }}"
  _hints: "{{ item.get('spec', {}).get('rootDeviceHints', {}) }}"
  _hint_wwn: "{{ _hints.get('wwn', '') }}"
  _storage: "{{ item.get('status', {}).get('hardware', {}).get('storage', []) }}"
```

### 17e. Structured section append

Every readiness role appends one section dict to `readiness_sections`. The final gate publishes
these sections as the `readiness_report` artifact.

```yaml
- name: Append root disk RAID backing section to readiness_sections
  vars:
    _pass: "{{ _root_disk_raid_results | selectattr('status', 'eq', 'PASS') | list | length }}"
    _warn: "{{ _root_disk_raid_results | selectattr('status', 'eq', 'WARN') | list | length }}"
    _fail: "{{ _root_disk_raid_results | selectattr('status', 'eq', 'FAIL') | list | length }}"
  ansible.builtin.set_fact:
    readiness_sections: >-
      {{
        readiness_sections + [{
          'section': 'root_disk_raid',
          'pass': _pass | int,
          'warn': _warn | int,
          'fail': _fail | int,
          'results': _root_disk_raid_results
        }]
      }}
```

## 18. File checklist for a new role

When creating `roles/readiness_check_<name>/`:

- [ ] `defaults/main.yml` — all configuration variables with inline comments explaining each
- [ ] `tasks/main.yml` — starts with `_results: []` reset; ends with console debug + failures append + readiness_sections append + readiness_warn_count increment
- [ ] `meta/main.yml` — `author`, `description`, `min_ansible_version: "2.14"`, `dependencies: []`
- [ ] `README.md` — documents all variables, all status codes, and requirements

When adding the role to the parent playbook:

- [ ] New play with `gather_facts: false`, `environment: KUBECONFIG: "{{ ocp_kubeconfig }}"`, tags

When the role queries a new resource type:

- [ ] Verify condition field names (check OCP API docs — they are not always consistent)
- [ ] Confirm OpenShift condition statuses are compared consistently, normally against `"True"`/`"False"` strings
- [ ] Test on a real cluster or mock JSON before committing

---

## 19. Quick reference: common mistakes

| Mistake | Correct pattern |
|---------|----------------|
| `_available == true` for OpenShift condition status | `_available == 'True'` |
| `'detail: ' + _bool_var` | `'detail: ' ~ _bool_var` |
| `known[item].key \| default(x)` | `known.get(item, {}).get('key', x)` |
| `environment: KUBECONFIG:` on each task | Set at play level; roles inherit it |
| `ansible.builtin.fail` inside a role | Append to `readiness_failures` only |
| No `_results: []` reset at top | Always reset before accumulating |
| `loop_control` omitted | Always set `label:` for readable output |
| `version: "operator.v1.2.3"` (dots unescaped) | `version: "operator\\.v1\\.2\\.3"` |
| `_csv_installed_raw.stdout_lines \| list` without dedup | `\| unique \| sort \| list` |
| No `changed_when: false` on `oc get` tasks | Always set for read-only commands |
| String concat with `+` in set_fact | Use `~` everywhere |
| `\| default()` to catch missing dict key | Use `.get('key', fallback)` |
| Appending Markdown report text from a role | Append a structured dict to `readiness_sections` |
| Forgetting to increment `readiness_warn_count` | Add `readiness_warn_count` update task after section append |
