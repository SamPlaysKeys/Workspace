---
type: Guide
status: Active
system: Ansible Automation Platform (AAP)
related_to:
  - artifacts/ansible/aap-template-export/
references:
  - https://docs.redhat.com/en/documentation/red_hat_ansible_automation_platform/
---

# Exporting and Enriching AAP Job Templates and Workflows

## Overview

Red Hat Ansible Automation Platform (AAP) and AWX Automation Controller store Job Templates and Workflow Job Templates with relational foreign keys (`project`, `inventory`, `execution_environment`) representing internal database IDs rather than human-readable names.

When performing audits, CMDB reconciliations, platform migrations, or reporting, operations teams need a unified catalog where all template metadata is enriched with the actual names of the referenced Projects, Inventories, and Execution Environments.

This guide details the API querying model, data enrichment architecture, and execution procedures using the standalone export automation tools in [`artifacts/ansible/aap-template-export/`](../../../artifacts/ansible/aap-template-export/).

---

## Architecture & API Flow

The export workflow executes across five core Controller v2 API endpoints:

```text
  +-------------------------------------------------------------------------+
  |                   AAP / Automation Controller v2 API                   |
  +-------------------------------------------------------------------------+
       |                           |                            |
       v                           v                            v
  /api/v2/projects/      /api/v2/inventories/     /api/v2/execution_environments/
  [ID -> Name Map]       [ID -> Name Map]             [ID -> Name Map]
       |                           |                            |
       +---------------------------+----------------------------+
                                   |
                                   v  (Enrich Foreign Keys)
      +----------------------------+----------------------------+
      |                                                         |
      v                                                         v
  /api/v2/job_templates/                    /api/v2/workflow_job_templates/
  (Standard Job Templates)                  (Workflow Job Templates)
      |                                                         |
      +----------------------------+----------------------------+
                                   |
                                   v
                      Unified Report (CSV / JSON)
```

### Endpoints Used

1. **`GET /api/v2/projects/?page_size=100`**: Builds a dictionary mapping Project ID $\rightarrow$ Project Name.
2. **`GET /api/v2/inventories/?page_size=100`**: Builds a dictionary mapping Inventory ID $\rightarrow$ Inventory Name.
3. **`GET /api/v2/execution_environments/?page_size=100`**: Builds a dictionary mapping Execution Environment ID $\rightarrow$ EE Name.
4. **`GET /api/v2/job_templates/?page_size=100`**: Queries standard templates containing playbook references, forks, verbosity, and foreign keys.
5. **`GET /api/v2/workflow_job_templates/?page_size=100`**: Queries workflow templates containing workflow-level inventory assignments.

---

## Prerequisites & Authentication

### 1. Controller Token
Generate an OAuth2 Bearer Token in AAP Controller:
- Navigate to **Users** $\rightarrow$ select your user $\rightarrow$ **Tokens** $\rightarrow$ **Add**.
- Ensure the user account has at least `Auditor` or `Read` permissions across the target Organizations, Projects, Inventories, and Templates.

### 2. Client Dependencies
- **Bash Deliverables**: `bash` (4.0+ recommended), `curl`, `jq`.
- **Python POC**: Python 3.6+ standard library (no pip packages needed).

---

## Execution Options

Reusable scripts are stored in `artifacts/ansible/aap-template-export/`:

### Option A: Comprehensive Production Bash Script (`aap_template_export.sh`)

Features full automatic pagination recursion over `.next` links, CLI argument parsing, endpoint 404 resilience, and multi-format support.

```bash
# Export as CSV with self-signed TLS bypass
./artifacts/ansible/aap-template-export/aap_template_export.sh \
  -h https://controller.example.com \
  -t "YOUR_BEARER_TOKEN" \
  -k \
  -f csv \
  -o aap_report.csv

# Export as JSON using environment variables
export AAP_HOST="https://controller.example.com"
export AAP_TOKEN="YOUR_BEARER_TOKEN"
./artifacts/ansible/aap-template-export/aap_template_export.sh -k -f json -o aap_report.json
```

### Option B: Minimal Quick-Run Script (`aap_template_export_mini.sh`)

Lightweight (~45-line) script tailored for quick interactive exports producing a 5-column CSV (`Template Name,Template Type,Execution Environment,Project,Inventory`):

```bash
AAP_URL="https://controller.example.com" AAP_TOKEN="YOUR_BEARER_TOKEN" \
  ./artifacts/ansible/aap-template-export/aap_template_export_mini.sh
```

### Option C: Python Reference PoC (`aap_template_export.py`)

> ⚠️ **Note:** Reference Proof-of-Concept only (not tested in production). Useful for building custom Python automation pipelines.

```bash
python3 ./artifacts/ansible/aap-template-export/aap_template_export.py \
  --host https://controller.example.com \
  --token "YOUR_BEARER_TOKEN" \
  -k \
  -f csv \
  -o aap_report.csv
```

---

## Output Data Schema

| Field | Description | Standard Job Template | Workflow Job Template |
| :--- | :--- | :--- | :--- |
| `id` | Unique ID in Controller | Integer | Integer |
| `name` | Name of the template | String | String |
| `type` | Classification | `job_template` | `workflow` |
| `job_type` | Job run mode | `run`, `check`, `scan` | `N/A` |
| `project_id` | Project ID | Integer / Empty | `null` |
| `project_name` | Resolved Project Name | String / `""` | `N/A` |
| `inventory_id` | Inventory ID | Integer / Empty | Integer / Empty |
| `inventory_name` | Resolved Inventory Name | String / `""` | String / `N/A` |
| `execution_environment_id` | EE ID | Integer / Empty | `null` |
| `execution_environment_name` | Resolved EE Name | String / `""` | `N/A` |
| `playbook` | Root playbook path | String | `N/A` |
| `forks` | Concurrency limit | Integer | `N/A` |
| `verbosity` | Log level (0-5) | Integer | `N/A` |

---

## Common Pitfalls & Operational Considerations

### 1. Shell Compatibility (`bash` vs `sh`)
- `set -euo pipefail` and Bash array expansions require GNU Bash.
- If invoked via `sh ./script.sh`, standard POSIX shells like `dash` or `ash` will error with `Illegal option -o pipefail`. Always execute directly (`./script.sh`) or with `bash ./script.sh`.

### 2. Missing or Optional Endpoints (HTTP 404)
- Legacy AWX or Ansible Tower instances without Execution Environments (`/api/v2/execution_environments/`) may return HTTP 404.
- `aap_template_export.sh` gracefully intercepts 404 responses for optional endpoints and continues processing without failing the export.

### 3. Special Characters and RFC 4180 CSV Escaping
- Job template names frequently contain commas, quotes, and brackets.
- The scripts pipe output through `jq`'s `@csv` filter or Python's `csv.DictWriter` to ensure RFC 4180 compliance and prevent column shifting during spreadsheet import.

### 4. Controller API Pagination
- AAP defaults to 20 items per page if unspecified. Querying large platforms requires traversing `.next` pagination URLs until exhausted. `aap_template_export.sh` recursively traverses all pages.

---

## References

- [Red Hat Ansible Automation Platform Controller API Guide](https://docs.redhat.com/en/documentation/red_hat_ansible_automation_platform/) - Official AAP v2 REST API documentation.
- [Reusable Exporter Artifacts](../../../artifacts/ansible/aap-template-export/README.md) - Source code and templates for the exporter tools.
