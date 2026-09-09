# AAP Inventory & Job Template Exporter

This directory contains tools to query Ansible Automation Platform (AAP) / Automation Controller v2 APIs, enrich Job Templates and Workflow Job Templates with human-readable Project, Inventory, and Execution Environment names, and output unified CSV or JSON reports.

## Features
- **Comprehensive Template Coverage:** Pulls both standard Job Templates (`/api/v2/job_templates/`) and Workflow Job Templates (`/api/v2/workflow_job_templates/`).
- **Full Pagination Handling:** Recursively follows `.next` links across all API endpoints to ensure no items are truncated or missed.
- **Data Enrichment:** Resolves `project_id` $\rightarrow$ `project_name`, `inventory_id` $\rightarrow$ `inventory_name`, and `execution_environment_id` $\rightarrow$ `execution_environment_name`.
- **RFC 4180 CSV Safety:** Safely escapes complex template names containing quotes, commas, and special characters.
- **Multiple Output Formats:** Direct output to stdout or file in `csv` or `json`.

## Contents
- **`aap_inventory_export.sh`** (Comprehensive Bash Deliverable): Drop-in Bash script featuring full recursive pagination (`.next`), CLI flag parsing, and multi-format support (`csv`/`json`).
- **`aap_inventory_export_mini.sh`** (Minimal Quick-Run Script): Lean ~40-line standalone script for direct 5-column CSV export (`Template Name,Template Type,Execution Environment,Project,Inventory`) with automatic temp cleanup.
- **`aap_inventory_export.py`** (Proof-of-Concept): Standalone Python 3 script using standard library modules (zero external pip dependencies).
- **`context.md`**: Storm session context and planning notes.

## Quick Start

### Bash Script
```bash
# Via CLI arguments
./aap_inventory_export.sh -h https://controller.example.com -t "YOUR_BEARER_TOKEN" -k -f csv -o export.csv

# Via Environment Variables
export AAP_HOST="https://controller.example.com"
export AAP_TOKEN="YOUR_BEARER_TOKEN"
./aap_inventory_export.sh -k -f json -o export.json
```

### Python POC Script
```bash
./aap_inventory_export.py --host https://controller.example.com --token "YOUR_BEARER_TOKEN" -k -f csv -o export.csv
```

## Options
- `-h`, `--host`: AAP Controller Host URL
- `-t`, `--token`: AAP API OAuth2 Bearer Token
- `-k`, `--insecure`: Disable SSL certificate verification (useful for internal/self-signed certs)
- `-f`, `--format`: Output format (`csv` or `json`, default: `csv`)
- `-o`, `--output`: Target output file path (defaults to stdout)

## Output Schema
| Column | Description |
| :--- | :--- |
| `id` | Unique ID of the template / workflow |
| `name` | Human-readable name of the template / workflow |
| `type` | `job_template` or `workflow` |
| `job_type` | Job type (`run`, `check`, `scan`, or `N/A` for workflows) |
| `project_id` | Project ID (or null/empty for workflows) |
| `project_name` | Project name (or `N/A` for workflows) |
| `inventory_id` | Associated Inventory ID |
| `inventory_name` | Associated Inventory name |
| `execution_environment_id` | Associated Execution Environment ID |
| `execution_environment_name` | Associated Execution Environment name |
| `playbook` | Playbook filename (or `N/A` for workflows) |
| `forks` | Concurrency forks count |
| `verbosity` | Log verbosity level |
