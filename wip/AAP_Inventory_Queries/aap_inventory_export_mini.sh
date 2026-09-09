#!/usr/bin/env bash
# ==============================================================================
# AAP Inventory Quick Exporter (Minimal Deliverable)
# Queries AAP / AWX API v2 for Job Templates and Workflow Templates,
# resolves IDs to human-readable names, and outputs an RFC 4180 CSV report.
# ==============================================================================
set -euo pipefail

AAP_URL="${AAP_URL:-https://your-aap-controller-url}"
AAP_TOKEN="${AAP_TOKEN:-your_oauth2_token_here}"
OUTPUT_FILE="${OUTPUT_FILE:-aap_report.csv}"

# Isolated temporary workspace with automatic cleanup
TMP_DIR="$(mktemp -d)"
trap 'rm -rf "$TMP_DIR"' EXIT

AUTH_HEADER="Authorization: Bearer $AAP_TOKEN"
CURL_OPTS=(-s -k -H "$AUTH_HEADER")

echo "Fetching AAP metadata..."
curl "${CURL_OPTS[@]}" "$AAP_URL/api/v2/execution_environments/?page_size=200" | jq -r '.results[]? | {id: .id, name: .name}' > "$TMP_DIR/ee.json"
curl "${CURL_OPTS[@]}" "$AAP_URL/api/v2/projects/?page_size=200"               | jq -r '.results[]? | {id: .id, name: .name}' > "$TMP_DIR/projects.json"
curl "${CURL_OPTS[@]}" "$AAP_URL/api/v2/inventories/?page_size=200"          | jq -r '.results[]? | {id: .id, name: .name}' > "$TMP_DIR/inventories.json"
curl "${CURL_OPTS[@]}" "$AAP_URL/api/v2/job_templates/?page_size=200"        > "$TMP_DIR/job_templates.json"
curl "${CURL_OPTS[@]}" "$AAP_URL/api/v2/workflow_job_templates/?page_size=200" > "$TMP_DIR/workflow_templates.json"

echo "Generating CSV Report..."
echo "Template Name,Template Type,Execution Environment,Project,Inventory" > "$OUTPUT_FILE"

# Standard Job Templates
jq -r --slurpfile ee "$TMP_DIR/ee.json" --slurpfile proj "$TMP_DIR/projects.json" --slurpfile inv "$TMP_DIR/inventories.json" '
  INDEX($ee[]; .id) as $ee_map |
  INDEX($proj[]; .id) as $proj_map |
  INDEX($inv[]; .id) as $inv_map |
  .results[]? |
  [
    .name,
    "Standard Job Template",
    ($ee_map[.execution_environment | tostring]?.name // "Default/None"),
    ($proj_map[.project | tostring]?.name // "None"),
    ($inv_map[.inventory | tostring]?.name // "None")
  ] | @csv
' "$TMP_DIR/job_templates.json" >> "$OUTPUT_FILE"

# Workflow Job Templates
jq -r --slurpfile inv "$TMP_DIR/inventories.json" '
  INDEX($inv[]; .id) as $inv_map |
  .results[]? |
  [
    .name,
    "Workflow Job Template",
    "N/A",
    "N/A",
    ($inv_map[.inventory | tostring]?.name // "None")
  ] | @csv
' "$TMP_DIR/workflow_templates.json" >> "$OUTPUT_FILE"

echo "Done! Report saved to $OUTPUT_FILE"
