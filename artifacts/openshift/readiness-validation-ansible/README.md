---
type: README-Note
---

# Readiness Check Role Authoring Guide

**Workspace path:** `artifacts/openshift/readiness-validation-ansible/`  
**What this is:** A reusable pattern for **Ansible readiness / validation roles** against live OpenShift clusters—multi-play parent playbook, shared Markdown report, **PASS / WARN / FAIL** semantics, and **deferred failure** (roles never call `fail`; a final gate play does). Includes skeletons and example roles.

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
8. [Dual reporting: console + markdown](#8-dual-reporting-console--markdown)
9. [Critical Ansible gotchas (hard-won fixes)](#9-critical-ansible-gotchas-hard-won-fixes)
10. [Reading OpenShift API condition fields](#10-reading-openshift-api-condition-fields)
11. [CSV version matching rules](#11-csv-version-matching-rules)
12. [KUBECONFIG management](#12-kubeconfig-management)
13. [Command task hygiene](#13-command-task-hygiene)
14. [Dictionary access safety](#14-dictionary-access-safety)
15. [Loop and output hygiene](#15-loop-and-output-hygiene)
16. [Failure deferral — never fail inside a role](#16-failure-deferral--never-fail-inside-a-role)
17. [File checklist for a new role](#17-file-checklist-for-a-new-role)
18. [Quick reference: common mistakes](#18-quick-reference-common-mistakes)

---

## 1. What is a readiness check role?

A readiness check role is an Ansible role that:

- Queries a live OpenShift cluster (or related infrastructure) to evaluate some aspect of
  cluster health or configuration
- Produces a structured result list with `PASS`, `WARN`, or `FAIL` entries
- Appends a human-readable section to a shared Markdown report fact (`readiness_report_md`)
- Records any failures in a shared `readiness_failures` list (but **never calls `fail` itself**)
- Is invoked from a multi-play parent playbook that runs a final gate play at the end

The role itself is a pure unit of checking — not of failing. Failure is centralised.

---

## 2. Playbook architecture

The parent playbook uses a **multi-play structure**:

```
Play 1 — Bootstrap  : add bastion to dynamic group
Play 2 — Initialize : set shared facts (kubeconfig path, empty readiness_failures, report header)
Play N — Role play  : gather_facts: false, environment: KUBECONFIG, one role
...
Play N+1 — Final gate: append summary to report, publish via set_stats, fail if readiness_failures
```

### Why this structure?

- **Deferred failure**: All roles run even if one fails. The final gate collects all failures
  into one clean summary — not 12 separate task failures mid-play.
- **Play-level environment**: Setting `environment: KUBECONFIG` at the play level means every
  `oc` command in every task inside the role automatically inherits it. Zero per-task repetition.
- **Shared facts**: `readiness_failures` and `readiness_report_md` are host facts set in Play 2
  and mutated by each role play. Because they are host facts (not inventory vars), they persist
  across play boundaries on the same host.
- **Isolation**: Each role play has `gather_facts: false`, so there is no per-role setup overhead.

### Template: parent playbook skeleton

See `templates/parent_playbook.yml`.

---

## 3. Role anatomy

Every readiness check role must contain exactly these files:

```
roles/readiness_<check_name>/
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

All readiness check roles are prefixed `readiness_`:

```
readiness_<check_name>
```

Examples: `readiness_check_cluster_operators`, `readiness_alert_routing`.

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

These two facts are set in the **initialize play** and consumed/extended by every role:

| Fact | Type | Set by | Extended by |
|------|------|--------|-------------|
| `readiness_failures` | list of strings | initialize play | each role (append on failure) |
| `readiness_report_md` | string (Markdown) | initialize play | each role (string concatenation) |

A third fact (`ocp_kubeconfig`) is also set in the initialize play and used at the play level:

```yaml
ocp_kubeconfig: "/home/{{ ansible_user | lower }}/{{ openshift_cluster_name }}/auth/kubeconfig"
```

**Rules:**
- Roles read `readiness_failures | default([])` (never assume it exists)
- Roles only **append** to `readiness_failures` — never replace it
- Roles only **concatenate** onto `readiness_report_md` — never replace it
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

## 8. Dual reporting: console + markdown

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

### 8b. Markdown section appended to readiness_report_md

Structure a `vars: _section:` block and then append it:

```yaml
- name: Append <check> section to readiness report
  vars:
    _section: |

      ## Section Title

      | Status | Name | Detail |
      |--------|------|--------|
      {% for r in (_my_results | sort(attribute='name')) -%}
      | {{ r.status }} | {{ r.name }} | {{ r.detail }} |
      {% endfor %}
      **PASS:** {{ _my_results | selectattr('status', 'eq', 'PASS') | list | length }} &nbsp;
      **WARN:** {{ _my_results | selectattr('status', 'eq', 'WARN') | list | length }} &nbsp;
      **FAIL:** {{ _my_results | selectattr('status', 'eq', 'FAIL') | list | length }}

  ansible.builtin.set_fact:
    readiness_report_md: "{{ readiness_report_md + _section }}"
```

**Note the `-%}` (dash) in the for loop** — it suppresses the trailing newline after each
iteration, preventing blank rows in the rendered Markdown table.

---

## 9. Critical Ansible gotchas (hard-won fixes)

These bugs were discovered in production and fixed through multiple commits. Every new role
author must know them.

### 9a. OCP API boolean coercion (the #1 bug source)

**The problem**: OpenShift API condition status fields return the strings `"True"` and `"False"`.
When Ansible parses JSON from `oc ... -o json` and sets a fact from it, it coerces these
strings to Python boolean `True` / `False`.

**Consequence**: Comparisons like `_available == 'True'` will **never match** a bool.
String concatenation like `'Available=' + _available` will **raise a TypeError** in Jinja2.

**The fix**:

```yaml
# WRONG — comparing strings to a bool:
status: (_available == 'True') | ternary('PASS', 'FAIL')

# CORRECT — compare against YAML boolean literals:
status: (_available == true) | ternary('PASS', 'FAIL')
```

```yaml
# WRONG — string + bool raises TypeError:
detail: 'Available=' + _available

# CORRECT — use ~ (tilde) for string concatenation:
detail: 'Available=' ~ _available
```

### 9b. Always use `~` for string concatenation in Jinja2 / set_fact

`+` is Python's concatenation operator and only works when both operands are already strings.
After bool coercion, status values are Python booleans — `+` raises a `TypeError`.

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
# CORRECT — compare against YAML boolean because Ansible coerces 'True'/'False':
status: (_available == false or _degraded == true) | ternary('FAIL', 'PASS')
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
    readiness_report_md: "# Cluster Readiness Report\n..."
```

**In each role play:**

```yaml
- name: Readiness validation — my check
  hosts: bastion_group
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

## 17. File checklist for a new role

When creating `roles/readiness_<name>/`:

- [ ] `defaults/main.yml` — all configuration variables with inline comments explaining each
- [ ] `tasks/main.yml` — starts with `_results: []` reset; ends with report debug + failures append + markdown append
- [ ] `meta/main.yml` — `author`, `description`, `min_ansible_version: "2.14"`, `dependencies: []`
- [ ] `README.md` — documents all variables, all status codes, and requirements

When adding the role to the parent playbook:

- [ ] New play with `gather_facts: false`, `environment: KUBECONFIG: "{{ ocp_kubeconfig }}"`, tags

When the role queries a new resource type:

- [ ] Verify condition field names (check OCP API docs — they are not always consistent)
- [ ] Confirm you compare boolean conditions against `true`/`false` (YAML booleans), not strings
- [ ] Test on a real cluster or mock JSON before committing

---

## 18. Quick reference: common mistakes

| Mistake | Correct pattern |
|---------|----------------|
| `_available == 'True'` | `_available == true` |
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
| `| default()` to catch missing dict key | Use `.get('key', fallback)` |
