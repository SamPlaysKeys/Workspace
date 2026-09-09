# AAP Inventory Queries — Context & Planning

## Objective
Develop scripts to query Ansible Automation Platform (AAP) / AWX Controller v2 API to retrieve standard job templates, workflow job templates, projects, and inventories, and produce an enriched inventory report with human-readable names.

## Deliverables
1. **Bash Standalone Script (`aap_inventory_export.sh`)**: Primary deliverable utilizing `curl` and `jq` with recursive pagination and configurable CLI parameters.
2. **Minimal Bash Script (`aap_inventory_export_mini.sh`)**: Lightweight (~40-line) single-command script focused on quick 5-column CSV export with clean temp directory isolation.
3. **Python Proof-of-Concept (`aap_inventory_export.py`)**: Zero-dependency Python script using standard library `urllib` to handle pagination, dictionary mapping, and CSV/JSON output.

## API Endpoints & Logic
- **Projects**: `/api/v2/projects/?page_size=100` $\rightarrow$ Map `id` to `name`
- **Inventories**: `/api/v2/inventories/?page_size=100` $\rightarrow$ Map `id` to `name`
- **Execution Environments**: `/api/v2/execution_environments/?page_size=100` $\rightarrow$ Map `id` to `name`
- **Job Templates**: `/api/v2/job_templates/?page_size=100` $\rightarrow$ Extract `id`, `name`, `job_type`, `project`, `inventory`, `playbook`, `execution_environment`, etc.
- **Workflow Job Templates**: `/api/v2/workflow_job_templates/?page_size=100` $\rightarrow$ Extract `id`, `name`, `inventory`, etc.
- **Pagination**: Handle API `.next` links across all paginated endpoints.

## Environment Variables / Arguments
- `AAP_HOST` / `CONTROLLER_HOST`
- `AAP_TOKEN` / `CONTROLLER_TOKEN`
- `INSECURE` / `-k` (disable TLS check)
- `--format` (`csv` or `json`)
