# Troubleshooting: Ansible Boolean Coercion with OCP/Kubernetes API Values

## TL;DR

Ansible coerces `"True"`/`"False"` strings from k8s condition fields to Python booleans. This breaks string concatenation and comparisons.

**Fixes:**
- Use `~` instead of `+` for string concatenation: `'Available=' ~ _available`
- Compare against `true`/`false` (YAML booleans), not `'True'`/`'False'` (strings)
- Handle `'Unknown'` as a string (it's not coerced)

---

## Background

The OpenShift and Kubernetes APIs represent condition status values as the **strings** `"True"` or
`"False"` (capital T/F) in JSON — for example:

```json
{
  "type": "Available",
  "status": "True"
}
```

When Ansible extracts these values via a Jinja2 expression and assigns them to a task-scoped
variable, it silently **coerces the strings to Python booleans** (`True` / `False`). This is a
well-known Ansible quirk: any variable whose resolved value looks like a YAML boolean
(`True`, `False`, `yes`, `no`, `on`, `off`) gets type-coerced, regardless of how the value
was originally defined in the source data.

---

## Common Signs You Have This Problem

### 1. `can only concatenate str (not "bool") to str`

The most direct symptom. You will see this in the failed task's `msg`:

```
{% raw %}
Unexpected templating type error occurred on ({{ ... }}):
can only concatenate str (not "bool") to str.
{% endraw %}
```

This happens when a Jinja2 expression uses `+` to build a string and one of the variables
is a Python bool instead of a string:

```yaml
# BROKEN — _available is a bool, not a string
'detail': 'Available=' + _available + '  Degraded=' + _degraded
```

### 2. Status checks always evaluate the same way (silent logic bug)

Even when the template doesn't crash, comparisons like `_available == 'True'` silently
**never match** because Python `True == 'True'` is `False`. Your status logic will always
fall through to the default branch:

```yaml
# BROKEN — bool True never equals the string 'True'
(_available == 'True') | ternary('PASS', 'FAIL')
# → always produces 'FAIL', even when the operator IS available
```

This is more dangerous than the crash case because it produces wrong results without any error.

### 3. Affects any variable extracted from OCP/k8s condition status fields

The pattern at risk:

```yaml
{% raw %}
_available: >-
  {{ item.status.conditions
     | selectattr('type', 'eq', 'Available')
     | map(attribute='status') | first | default('Unknown') }}
{% endraw %}
```

Any time the above resolves to the bare strings `"True"` or `"False"`, Ansible coerces them.
This applies to ClusterOperators, Node conditions, Pod conditions, and any other k8s resource
that uses the standard condition schema.

---

## Why It Happens

Ansible processes variable values through its type-coercion layer after Jinja2 evaluation.
When a template expression like {% raw %}`{{ ... | first }}`{% endraw %} returns the string `"True"` and that is
the **entire value** of the variable (no surrounding text), Ansible recognises it as a YAML
boolean and converts it to the Python object `True`.

Using a YAML block scalar (`>-`) does **not** protect against this — the coercion happens
after YAML parsing, during Ansible's variable resolution.

---

## How to Fix

### Fix 1: Use `~` (tilde) instead of `+` for string concatenation

Jinja2's `~` operator converts each operand to a string before concatenating. It is immune
to the bool coercion problem:

```yaml
# FIXED
'detail': (
  'Available=' ~ _available
  ~ '  Progressing=' ~ _progressing
  ~ '  Degraded=' ~ _degraded
)
```

Use `~` any time you are building a string from variables that may have been sourced from
a k8s/OCP condition status field.

### Fix 2: Compare against YAML boolean literals, not strings

Change comparisons from string literals `'True'`/`'False'` to YAML boolean literals
`true`/`false`:

```yaml
# BROKEN — string comparison against a bool always returns False
(_available == 'False' or _degraded == 'True')

# FIXED — boolean comparison works correctly
(_available == false or _degraded == true)
```

In Jinja2/Python:
- `False == false` → `True` ✓
- `True == true`  → `True` ✓
- `'Unknown' == false` → `False` (safe — unknown is treated as not-failed) ✓

### Fix 3 (alternative): Force the variable to remain a string

If you need string semantics throughout (e.g., you want to display `"True"` not `"true"`),
you can prevent coercion by embedding the value inside a larger string expression so it
never resolves to a bare boolean:

```yaml
{% raw %}
# Wrap in a format string — Ansible will not coerce a multi-token string
_available: "status={{ item.status.conditions | ... | first | default('Unknown') }}"
{% endraw %}
```

This is more of a workaround than a fix; prefer Fix 1 + Fix 2 for clarity.

---

## Quick Reference: OCP Condition Status → Expected Behavior

| Field | API value | Python after coercion | Correct comparison |
|---|---|---|---|
| `Available` = healthy | `"True"` | `True` | `_available == true` |
| `Available` = unhealthy | `"False"` | `False` | `_available == false` or `not _available` |
| `Progressing` = converging | `"True"` | `True` | `_progressing == true` or `_progressing` |
| `Degraded` = healthy | `"False"` | `False` | `_degraded == false` or `not _degraded` |
| `Degraded` = unhealthy | `"True"` | `True` | `_degraded == true` or `_degraded` |
| Any field = missing | `"Unknown"` | `"Unknown"` (string, not coerced) | `_available == 'Unknown'` |

> **Note:** `"Unknown"` is not a YAML boolean keyword, so it is **not** coerced and remains a
> string. Your logic must handle all three cases: bool `True`, bool `False`, string `'Unknown'`.

---

## Example: Full Corrected Pattern

```yaml
{% raw %}
- name: Evaluate each ClusterOperator
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
  ansible.builtin.set_fact:
    _results: >-
      {{
        _results + [{
          'name': item.metadata.name,
          'status': (
            (_available == false or _degraded == true)
            | ternary('FAIL',
              (_progressing == true)
              | ternary('WARN', 'PASS')
            )
          ),
          'detail': (
            'Available=' ~ _available
            ~ '  Progressing=' ~ _progressing
            ~ '  Degraded=' ~ _degraded
          )
        }]
      }}
  loop: "{{ _items }}"
{% endraw %}
```

---

## Checklist When Writing New Roles That Query OCP/k8s Conditions

- [ ] Use `~` instead of `+` for any string built from condition status vars
- [ ] Compare condition status vars against `true`/`false` (YAML booleans), not `'True'`/`'False'` (strings)
- [ ] Account for the `'Unknown'` case (a string, not a bool) in status logic
- [ ] Test against a cluster where at least one operator is degraded or progressing to exercise all branches
