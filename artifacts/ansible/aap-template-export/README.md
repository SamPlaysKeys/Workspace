---
type: README-Note
---

# AAP / AWX Job Template & Workflow Exporters

A suite of production-ready automation scripts and reference tools designed to query Red Hat Ansible Automation Platform (AAP) / AWX Controller v2 APIs, extract Job Templates and Workflow Job Templates, resolve foreign IDs to human-readable names (Projects, Inventories, Execution Environments), and export unified CSV or JSON reports.

---

## Contents & Variants

| Script | Type | Best For | Dependencies | Description |
| :--- | :--- | :--- | :--- | :--- |
| **`aap_template_export.sh`** | Bash (Production) | Production CLI / Cron / Pipelines | `bash`, `curl`, `jq` | Full-featured deliverable with automatic pagination recursion over `.next`, robust CLI flags, endpoint 404 resilience, and CSV/JSON output formats. |
| **`aap_template_export_mini.sh`** | Bash (Minimal) | Quick ad-hoc audits / interactive runs | `bash`, `curl`, `jq` | Lightweight (~45 line) single-pass script generating a 5-column CSV (`Template Name,Template Type,Execution Environment,Project,Inventory`) with isolated temp workspace. |
| **`aap_template_export.py`** | Python 3 (PoC) | Prototyping & reference architecture | Python 3 (standard library only) | **Proof-of-Concept only (not tested in production)**. Zero-dependency implementation demonstrating `urllib`, `json`, `csv`, and `ssl` pagination logic. |

---

## Quick Start

### 1. Comprehensive Bash Exporter (`aap_template_export.sh`)

```bash
# Run with CLI arguments
./aap_template_export.sh -h https://controller.example.com -t "YOUR_BEARER_TOKEN" -k -f csv -o aap_report.csv

# Run with environment variables
export AAP_HOST="https://controller.example.com"
export AAP_TOKEN="YOUR_BEARER_TOKEN"
./aap_template_export.sh -k -f json -o aap_report.json
```

**Options:**
- `-h`, `--host URL`: AAP Controller host URL.
- `-t`, `--token TOKEN`: OAuth2 Bearer token.
- `-k`, `--insecure`: Disable SSL certificate verification (for self-signed controller certs).
- `-f`, `--format FORMAT`: Output format (`csv` or `json`, default: `csv`).
- `-o`, `--output FILE`: Output file path (defaults to standard output).
- `--help`: Display usage help.

---

### 2. Minimal Bash Exporter (`aap_template_export_mini.sh`)

```bash
# Run with environment variables
AAP_URL="https://controller.example.com" AAP_TOKEN="YOUR_BEARER_TOKEN" ./aap_template_export_mini.sh
```

Outputs `aap_report.csv` directly in the current directory with columns:
`Template Name,Template Type,Execution Environment,Project,Inventory`

---

### 3. Python Proof-of-Concept (`aap_template_export.py`)

> ⚠️ **Disclaimer:** This script is a reference Proof-of-Concept and has **not** been tested in production environments.

```bash
python3 ./aap_template_export.py --host https://controller.example.com --token "YOUR_BEARER_TOKEN" -k -f csv -o report.csv
```

---

## Output Schema (Comprehensive Scripts)

When exporting with `aap_template_export.sh` or `aap_template_export.py`, the following columns are generated:

| Column | Type | Description | Standard JT | Workflow JT |
| :--- | :--- | :--- | :--- | :--- |
| `id` | Integer | Controller Template ID | populated | populated |
| `name` | String | Template Name | populated | populated |
| `type` | String | Template classification | `job_template` | `workflow` |
| `job_type` | String | Job launch type | `run` / `check` / `scan` | `N/A` |
| `project_id` | Integer / Null | Underlying Project ID | populated / null | `null` |
| `project_name` | String | Resolved human-readable Project name | resolved / `""` | `N/A` |
| `inventory_id` | Integer / Null | Associated Inventory ID | populated / null | populated / null |
| `inventory_name` | String | Resolved human-readable Inventory name | resolved / `""` | resolved / `N/A` |
| `execution_environment_id` | Integer / Null | Associated Execution Environment ID | populated / null | `null` |
| `execution_environment_name` | String | Resolved human-readable EE name | resolved / `""` | `N/A` |
| `playbook` | String | Entrypoint playbook filename | populated | `N/A` |
| `forks` | Integer / String | Task concurrency forks | integer | `N/A` |
| `verbosity` | Integer / String | Output logging verbosity level (0–5) | integer | `N/A` |

---

## API Endpoints Utilized

- `GET /api/v2/projects/?page_size=100` $\rightarrow$ Map `id` to `name`
- `GET /api/v2/inventories/?page_size=100` $\rightarrow$ Map `id` to `name`
- `GET /api/v2/execution_environments/?page_size=100` $\rightarrow$ Map `id` to `name`
- `GET /api/v2/job_templates/?page_size=100` $\rightarrow$ Core standard job templates
- `GET /api/v2/workflow_job_templates/?page_size=100` $\rightarrow$ Core workflow job templates
