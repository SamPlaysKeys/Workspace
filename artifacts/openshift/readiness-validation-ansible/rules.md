---
type: Reference
---

# Readiness Check Role — Authoring Rules

Condensed machine-readable ruleset for AI agents generating new readiness check roles.
See `README.md` for full explanation of each rule.

---

## ARCHITECTURE

**ARCH-1** — Parent playbook structure: Bootstrap play → Initialize play → one role per play → Final gate play.

**ARCH-2** — Each role play must have `gather_facts: false` and `environment: KUBECONFIG: "{{ ocp_kubeconfig }}"`.

**ARCH-3** — The initialize play sets three host facts: `ocp_kubeconfig`, `readiness_failures: []`, `readiness_report_md: "..."`.

**ARCH-4** — The final gate play appends a summary to `readiness_report_md`, publishes via `set_stats`, and calls `ansible.builtin.fail` if `readiness_failures | length > 0`.

**ARCH-5** — Every play has two tags: the global suite tag and a play-specific check tag.

---

## ROLE STRUCTURE

**ROLE-1** — Every role contains exactly: `defaults/main.yml`, `tasks/main.yml`, `meta/main.yml`, `README.md`.

**ROLE-2** — Role name prefix: `readiness_<check_name>`.

**ROLE-3** — Internal (role-private) fact names use a leading underscore: `_co_results`, `_amtool_raw`.

**ROLE-4** — `defaults/main.yml` contains all operator/config baselines; inline comments must explain each variable.

**ROLE-5** — `meta/main.yml` must set `min_ansible_version: "2.14"` and `dependencies: []`.

**ROLE-6** — `README.md` must document every variable, every status code, and requirements.

---

## SHARED STATE CONTRACT

**STATE-1** — Roles read `readiness_failures | default([])` — never assume the fact exists.

**STATE-2** — Roles only append to `readiness_failures`; never replace it.

**STATE-3** — Roles only concatenate onto `readiness_report_md`; never replace it.

**STATE-4** — Roles never read or write `ocp_kubeconfig`.

---

## RESULT ACCUMULATION

**ACCUM-1** — The first task in `tasks/main.yml` resets the local result list: `set_fact: _results: []`.

**ACCUM-2** — Results are accumulated via `set_fact: _results: "{{ _results + [{ ... }] }}"` in a loop.

**ACCUM-3** — Every result entry must have: `name` (string), `status` (PASS/WARN/FAIL), `detail` (string).

**ACCUM-4** — Result entries should carry a `required` boolean to drive PASS/WARN/FAIL logic.

---

## STATUS CODES

**STATUS-1** — Exactly three status values: `PASS`, `WARN`, `FAIL`. No other values.

**STATUS-2** — `FAIL` only when `required: true` and the check did not pass. Append to `readiness_failures`.

**STATUS-3** — `WARN` when `required: false` and the check did not pass, or for informational findings.

**STATUS-4** — Multi-branch status logic uses nested ternary: `(cond1) | ternary('FAIL', (cond2) | ternary('WARN', 'PASS'))`.

---

## BOOLEAN COERCION (critical — most common bug source)

**BOOL-1** — OpenShift API condition status fields (`"True"`/`"False"` strings) are coerced by Ansible to Python
booleans. Compare against YAML boolean literals only.

```yaml
# WRONG:
(_available == 'True')

# CORRECT:
(_available == true)
```

**BOOL-2** — Never use Python `+` operator for string concatenation in `set_fact` or `vars` blocks.
Use Jinja2 `~` (tilde). The `~` operator coerces both operands to strings; `+` raises
`TypeError` when one operand is a boolean.

```yaml
# WRONG:
detail: 'Available=' + _available + '  Degraded=' + _degraded

# CORRECT:
detail: 'Available=' ~ _available ~ '  Degraded=' ~ _degraded
```

---

## DICTIONARY ACCESS SAFETY

**DICT-1** — Use `.get('key', fallback)` for all nested dict lookups on facts derived from external data.
Jinja2 dot-notation on a plain Python dict raises `AttributeError` before `| default()` can intercept it.

```yaml
# WRONG:
known[item].explanation | default('unknown')

# CORRECT:
known.get(item, {}).get('explanation', 'unknown')
```

**DICT-2** — Chain `.get()` for two-level access: `outer.get('key', {}).get('inner_key', 'default')`.

---

## KUBECONFIG

**KUBE-1** — Set `ocp_kubeconfig` once in the initialize play as a host fact.

**KUBE-2** — Reference it at play level: `environment: KUBECONFIG: "{{ ocp_kubeconfig }}"`.

**KUBE-3** — Never set `KUBECONFIG` as a per-task environment variable in role task files.

---

## COMMAND TASK HYGIENE

**CMD-1** — All read-only `oc` commands must set `changed_when: false`.

**CMD-2** — Register names for raw command output use `_raw` suffix: `register: _co_raw`.

**CMD-3** — Parsed facts drop the `_raw` suffix: `_co_items: "{{ (_co_raw.stdout | from_json)['items'] }}"`.

---

## LOOP HYGIENE

**LOOP-1** — Every loop must set `loop_control: label:` to a meaningful string (e.g., `item.metadata.name`).

**LOOP-2** — Deduplicate list facts from API output with `| unique` before iterating: `| unique | sort | list`.

**LOOP-3** — When parsing tabular `stdout_lines`, skip the header: `stdout_lines[1:] | select('ne', '') | ...`.

---

## FAILURE DEFERRAL

**FAIL-1** — Roles never call `ansible.builtin.fail`. They append to `readiness_failures` only.

**FAIL-2** — Failure message format: `"<role_name>: N required check(s) failed — see <Report Title> above"`.

**FAIL-3** — Failure append is conditional: `when: _results | selectattr('status', 'eq', 'FAIL') | list | length > 0`.

---

## REPORTING

**REPORT-1** — Every role produces a console debug block using `ansible.builtin.debug: msg:` with a Jinja2 template.

**REPORT-2** — Console report format: `=== Title ===` header, aligned columns using `{{ '%-6s' | format(r.status) }}`, `=== Summary ===` footer with PASS/WARN/FAIL counts.

**REPORT-3** — Every role appends a Markdown section to `readiness_report_md` via `set_fact: readiness_report_md: "{{ readiness_report_md + _section }}"`.

**REPORT-4** — Markdown tables use `-%}` (dash) in for loops to suppress blank rows.

**REPORT-5** — Markdown summary line format: `**PASS:** N &nbsp; **WARN:** N &nbsp; **FAIL:** N`.

**REPORT-6** — Result tables are sorted by name: `_results | sort(attribute='name')`.

**REPORT-7** — The final gate play publishes the report via `ansible.builtin.set_stats: data: readiness_report_md: aggregate: false`.

---

## CSV VERSION MATCHING

**CSV-1** — CSV match uses anchored Python regex: `_csv_installed | select('match', '^' + version_pattern + '$') | list`.

**CSV-2** — All literal dots in version patterns must be escaped: `\\.` in YAML double-quoted strings.

**CSV-3** — Variable build suffixes (datestamps) use `\\d+` to match one or more digits.

**CSV-4** — The dict key (human-readable label) is display-only. The match is driven by the `version` regex field.

**CSV-5** — Detect version mismatches (wrong version installed) vs absent operators using `select('search', item.key)` close-match logic.

**CSV-6** — Always deduplicate the installed CSV list — OLM reports one CSV per namespace.

---

## OPENING CONDITION FIELDS (OCP API)

**OCP-1** — Extract condition values with:
```yaml
{{ item.status.conditions | selectattr('type', 'eq', 'ConditionType') | map(attribute='status') | first | default('Unknown') }}
```

**OCP-2** — Compare extracted values against YAML boolean literals (`true`/`false`), never strings.

**OCP-3** — Always provide `| default('Unknown')` on condition extraction — some resources may omit conditions.
