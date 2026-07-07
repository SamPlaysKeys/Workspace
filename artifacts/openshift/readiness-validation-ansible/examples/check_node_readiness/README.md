---
type: README-Note
---

# readiness_check_node_readiness

Validates that every cluster node is in `Ready` state and free of resource pressure
conditions (`MemoryPressure`, `DiskPressure`, `PIDPressure`).

## What it does

1. Fetches all nodes via `oc get nodes -o json`
2. For each node, extracts the `Ready`, `MemoryPressure`, `DiskPressure`, and
   `PIDPressure` conditions from `status.conditions`
3. Evaluates each node: `PASS` if Ready=true and all pressures=false; `FAIL` otherwise
4. Optionally checks the total node count against `expected_node_count`
5. Appends a structured `node_readiness` section to `readiness_sections`

## Result statuses

| Status | Meaning |
|--------|---------|
| `PASS` | Node is Ready and has no resource pressure |
| `FAIL` | Node is NotReady or has MemoryPressure / DiskPressure / PIDPressure |

There is no `WARN` for individual node conditions — all node health conditions are required.

## Variables

### `expected_node_count` (optional)

Integer. If greater than 0, an additional check verifies that the total number of nodes
matches this value exactly. `FAIL` if the count does not match.

Default: `0` (count check disabled)

```yaml
expected_node_count: 6   # 3 control plane + 3 workers
```

### `node_expected_conditions` (optional)

List of condition checks to apply to each node. Rarely needs changing — the defaults
cover all standard node health conditions.

```yaml
node_expected_conditions:
  - type: "Ready"
    expected: true
    required: true
  - type: "MemoryPressure"
    expected: false
    required: true
```

## Implementation notes

### Boolean coercion (Rule BOOL-1)

OpenShift API returns condition status fields as the strings `"True"` and `"False"`.
This example compares those values as strings, matching the condition-handling pattern used by
`readiness_check_cluster_operators`.

### String concatenation (Rule BOOL-2)

The `detail` field uses Jinja2 `~` (tilde) for concatenation — not Python `+`. This is
keeps detail construction safe if any value is not already a string; `~` coerces both operands
to strings before joining.

## Requirements

- `oc` CLI available on target host with a valid kubeconfig
- Target host added to a bastion group; parent playbook sets `KUBECONFIG` at play level
