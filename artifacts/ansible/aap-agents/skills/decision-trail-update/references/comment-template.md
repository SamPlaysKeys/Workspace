## Decision trail update (AAP evidence — YYYY-MM-DD)

This updates prior decision-trail evidence for branch `<head_branch>` at head SHA `<head_sha>`.

### Recent branch changes

1. `<theme 1 summary>`
2. `<theme 2 summary>`
3. `<theme 3 summary>`

### Recent AAP evidence

| Job | SHA | Elapsed | Result | Evidence / takeaway |
|---|---|---:|---|---|
| [1001](https://<aap-host>/#/jobs/playbook/1001/output) | `abc1234` | 8.2m | failed | `<one-line finding>` |
| [1002](https://<aap-host>/#/jobs/playbook/1002/output) | `def5678` | 10.1m | successful | `<what this validates>` |

### What this validates now

- `<validated code path 1>`
- `<validated code path 2>`

### What remains untested

- `<untested path 1>`
- `<untested path 2>`

### Verification / QA signal

- **Status:** `pass|conditional|block|unknown`
- **Latest-code coverage:** `<head_sha>` exercised by run evidence: `yes|no|unknown`
- **Required before merge:** `<one concrete action or none>`
