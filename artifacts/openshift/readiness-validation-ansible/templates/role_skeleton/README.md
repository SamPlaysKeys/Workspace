---
type: README-Note
---

# readiness_<check_name>

<REPLACE: One-paragraph description of what this role validates and why it matters.>

## What it does

1. <REPLACE: Step 1>
2. <REPLACE: Step 2>
3. <REPLACE: Step 3 ...>

## Result statuses

| Status | Meaning |
|--------|---------|
| `PASS` | <REPLACE: what PASS means for this check> |
| `WARN` | <REPLACE: what WARN means> |
| `FAIL` | <REPLACE: what FAIL means and what the operator should do> |

## Variables

### `<check_items>` (required)

<REPLACE: description of the main configuration variable>

```yaml
<check_items>:
  - name: "example-item"
    required: true
    expected: "expected-value"
```

### `<other_variable>` (optional)

<REPLACE: description>

Default: `<value>`

## Requirements

- `oc` CLI available on the target host with a valid kubeconfig
- Target host: `<bastion_group>` (set via parent playbook dynamic group)
- <REPLACE: any other prerequisites>
