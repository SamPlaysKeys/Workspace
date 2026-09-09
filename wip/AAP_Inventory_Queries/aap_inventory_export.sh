#!/usr/bin/env bash
#
# Fetches Projects, Inventories, Execution Environments, Job Templates,
# and Workflow Job Templates from Ansible Automation Platform (AAP) /
# Automation Controller v2 API, enriches them with human-readable names,
# and outputs unified CSV or JSON reports.
#
# Requirements: bash, curl, jq

set -euo pipefail

HOST="${CONTROLLER_HOST:-${AAP_HOST:-}}"
TOKEN="${CONTROLLER_TOKEN:-${AAP_TOKEN:-}}"
CURL_OPTS=""
FORMAT="csv"
OUTPUT_FILE=""

usage() {
  cat <<EOF
Usage: $(basename "$0") [options]

Export and enrich AAP / AWX Job Templates and Workflow Job Templates.

Options:
  -h, --host URL      AAP Controller host URL (e.g. https://controller.example.com)
  -t, --token TOKEN   AAP API OAuth2 Bearer token
  -k, --insecure      Disable SSL verification (-k / --insecure for curl)
  -f, --format FORMAT Output format: csv or json (default: csv)
  -o, --output FILE   Output file path (default: stdout)
  --help              Display this help message

Environment Variables:
  AAP_HOST / CONTROLLER_HOST    AAP Host URL
  AAP_TOKEN / CONTROLLER_TOKEN  AAP API Token
EOF
  exit 1
}

# Parse Arguments
while [[ $# -gt 0 ]]; do
  case "$1" in
    -h|--host)
      HOST="$2"
      shift 2
      ;;
    -t|--token)
      TOKEN="$2"
      shift 2
      ;;
    -k|--insecure)
      CURL_OPTS="-k"
      shift
      ;;
    -f|--format)
      FORMAT="$2"
      shift 2
      ;;
    -o|--output)
      OUTPUT_FILE="$2"
      shift 2
      ;;
    --help)
      usage
      ;;
    *)
      echo "Unknown option: $1" >&2
      usage
      ;;
  esac
done

if [[ -z "$HOST" || -z "$TOKEN" ]]; then
  echo "Error: Both host (--host / AAP_HOST) and token (--token / AAP_TOKEN) are required." >&2
  usage
fi

# Ensure dependencies exist
for cmd in curl jq; do
  if ! command -v "$cmd" &>/dev/null; then
    echo "Error: Required command '$cmd' is not installed." >&2
    exit 1
  fi
done

# Normalize Host URL
HOST="${HOST%/}"
if [[ ! "$HOST" =~ ^https?:// ]]; then
  HOST="https://$HOST"
fi

# Temporary Directory Cleanup
TMP_DIR=$(mktemp -d)
trap 'rm -rf "$TMP_DIR"' EXIT

fetch_paged_results() {
  local endpoint="$1"
  local output_file="$2"
  local name="$3"
  local url="$HOST$endpoint"

  echo "[]" > "$output_file"
  echo "Fetching $name..." >&2

  while [[ -n "$url" && "$url" != "null" ]]; do
    local resp_file="$TMP_DIR/resp.json"
    
    local http_code
    http_code=$(curl -s $CURL_OPTS -w "%{http_code}" -o "$resp_file" \
      -H "Authorization: Bearer $TOKEN" \
      -H "Accept: application/json" \
      "$url")

    if [[ "$http_code" -ne 200 ]]; then
      # Non-200 code handling (skip non-critical endpoints if 404)
      if [[ "$http_code" -eq 404 ]]; then
        echo "Note: $name endpoint not found (HTTP 404), skipping..." >&2
        return 0
      fi
      echo "Error fetching $name from $url (HTTP $http_code)" >&2
      cat "$resp_file" >&2
      exit 1
    fi

    # Append results page
    jq -s '.[0] + (.[1].results // [])' "$output_file" "$resp_file" > "${output_file}.tmp"
    mv "${output_file}.tmp" "$output_file"

    # Get next page URL
    url=$(jq -r '.next // empty' "$resp_file")
    if [[ -n "$url" && "$url" != "null" && ! "$url" =~ ^https?:// ]]; then
      url="$HOST$url"
    fi
  done
}

# 1. Fetch Resources
fetch_paged_results "/api/v2/projects/?page_size=100" "$TMP_DIR/projects.json" "Projects"
fetch_paged_results "/api/v2/inventories/?page_size=100" "$TMP_DIR/inventories.json" "Inventories"
fetch_paged_results "/api/v2/execution_environments/?page_size=100" "$TMP_DIR/ee.json" "Execution Environments"
fetch_paged_results "/api/v2/job_templates/?page_size=100" "$TMP_DIR/job_templates.json" "Job Templates"
fetch_paged_results "/api/v2/workflow_job_templates/?page_size=100" "$TMP_DIR/workflow_job_templates.json" "Workflow Job Templates"

# 2. Enrich Templates with Lookup Names
jq -n \
  --slurpfile projects "$TMP_DIR/projects.json" \
  --slurpfile inventories "$TMP_DIR/inventories.json" \
  --slurpfile ees "$TMP_DIR/ee.json" \
  --slurpfile templates "$TMP_DIR/job_templates.json" \
  --slurpfile workflows "$TMP_DIR/workflow_job_templates.json" \
  '
  (($projects[0] // []) | map({key: (.id | tostring), value: .name}) | from_entries) as $p_map |
  (($inventories[0] // []) | map({key: (.id | tostring), value: .name}) | from_entries) as $i_map |
  (($ees[0] // []) | map({key: (.id | tostring), value: .name}) | from_entries) as $e_map |

  (
    (($templates[0] // []) | map({
      id: .id,
      name: .name,
      type: "job_template",
      job_type: (.job_type // "run"),
      project_id: .project,
      project_name: (if .project then ($p_map[.project | tostring] // "") else "" end),
      inventory_id: .inventory,
      inventory_name: (if .inventory then ($i_map[.inventory | tostring] // "") else "" end),
      execution_environment_id: .execution_environment,
      execution_environment_name: (if .execution_environment then ($e_map[.execution_environment | tostring] // "") else "" end),
      playbook: (.playbook // ""),
      forks: (.forks // 0),
      verbosity: (.verbosity // 0)
    }))
    +
    (($workflows[0] // []) | map({
      id: .id,
      name: .name,
      type: "workflow",
      job_type: "N/A",
      project_id: null,
      project_name: "N/A",
      inventory_id: .inventory,
      inventory_name: (if .inventory then ($i_map[.inventory | tostring] // "") else "N/A" end),
      execution_environment_id: null,
      execution_environment_name: "N/A",
      playbook: "N/A",
      forks: "N/A",
      verbosity: "N/A"
    }))
  )
' > "$TMP_DIR/enriched.json"

# 3. Format Output
DEST="${OUTPUT_FILE:-/dev/stdout}"

if [[ "$FORMAT" == "json" ]]; then
  jq '.' "$TMP_DIR/enriched.json" > "$DEST"
elif [[ "$FORMAT" == "csv" ]]; then
  jq -r '
    ["id", "name", "type", "job_type", "project_id", "project_name", "inventory_id", "inventory_name", "execution_environment_id", "execution_environment_name", "playbook", "forks", "verbosity"],
    (.[] | [.id, .name, .type, .job_type, .project_id, .project_name, .inventory_id, .inventory_name, .execution_environment_id, .execution_environment_name, .playbook, .forks, .verbosity])
    | @csv
  ' "$TMP_DIR/enriched.json" > "$DEST"
else
  echo "Error: Unsupported format '$FORMAT'. Use 'csv' or 'json'." >&2
  exit 1
fi

COUNT_JT=$(jq 'length' "$TMP_DIR/job_templates.json")
COUNT_WF=$(jq 'length' "$TMP_DIR/workflow_job_templates.json")
TOTAL=$(jq 'length' "$TMP_DIR/enriched.json")
echo "Done! Exported $COUNT_JT job templates and $COUNT_WF workflows (Total: $TOTAL)." >&2
