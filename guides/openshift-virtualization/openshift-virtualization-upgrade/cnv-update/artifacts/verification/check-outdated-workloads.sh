#!/bin/bash
# Check for outdated virt-launcher workloads after CNV upgrade
#
# Usage: ./check-outdated-workloads.sh [options]
#
# Options:
#   -n, --namespace   Specific namespace to check (default: all namespaces)
#   -w, --watch       Watch mode (continuous monitoring)
#   -j, --json        Output in JSON format
#   -q, --quiet       Only show count, no details
#   -h, --help        Show this help message

set -e

NAMESPACE=""
WATCH_MODE=false
JSON_OUTPUT=false
QUIET_MODE=false

# Parse arguments
while [[ $# -gt 0 ]]; do
  case $1 in
    -n|--namespace)
      NAMESPACE="$2"
      shift 2
      ;;
    -w|--watch)
      WATCH_MODE=true
      shift
      ;;
    -j|--json)
      JSON_OUTPUT=true
      shift
      ;;
    -q|--quiet)
      QUIET_MODE=true
      shift
      ;;
    -h|--help)
      head -15 "$0" | tail -14
      exit 0
      ;;
    *)
      echo "Unknown option: $1"
      exit 1
      ;;
  esac
done

# Functions

check_outdated_count() {
  oc get kv kubevirt-kubevirt-hyperconverged -n openshift-cnv -o json 2>/dev/null | jq -r '.status.outdatedVirtualMachineInstanceWorkloads // "N/A"'
}

list_outdated_vmis() {
  local ns_arg=""
  if [[ -n "$NAMESPACE" ]]; then
    ns_arg="-n $NAMESPACE"
  else
    ns_arg="-A"
  fi
  
  oc get vmi -l kubevirt.io/outdatedLauncherImage $ns_arg --no-headers 2>/dev/null || true
}

check_operator_status() {
  oc get hco kubevirt-kubeconverged -n openshift-cnv -o json 2>/dev/null | jq -r '.status.conditions[] | select(.type == "Available") | .status'
}

check_active_migrations() {
  oc get vmim -A --no-headers 2>/dev/null | wc -l | tr -d ' '
}

format_output() {
  local count="$1"
  local vmis="$2"
  
  if [[ "$JSON_OUTPUT" == "true" ]]; then
    if [[ "$count" == "0" ]]; then
      echo '{"outdated_workloads": 0, "vmis": []}'
    else
      echo "$vmis" | awk '{print "{\"namespace\": \"" $1 "\", \"name\": \"" $2 "\"}"}' | jq -s '{"outdated_workloads": '"$count"', "vmis": .}'
    fi
  elif [[ "$QUIET_MODE" == "true" ]]; then
    echo "$count"
  else
    echo "=== Outdated Workloads ==="
    echo "Count: $count"
    echo ""
    if [[ "$count" != "0" && -n "$vmis" ]]; then
      echo "VMIs with outdated virt-launcher:"
      echo "$vmis" | awk '{print "  - " $1 "/" $2}'
    fi
  fi
}

# Main logic

main() {
  local count
  local vmis
  
  if [[ "$WATCH_MODE" == "true" ]]; then
    echo "Watching for outdated workloads... (Ctrl+C to stop)"
    echo ""
    while true; do
      count=$(check_outdated_count)
      clear
      echo "=== CNV Outdated Workloads Monitor ==="
      echo "Time: $(date '+%Y-%m-%d %H:%M:%S')"
      echo ""
      format_output "$count" "$(list_outdated_vmis)"
      echo ""
      echo "Active migrations: $(check_active_migrations)"
      sleep 5
    done
  else
    count=$(check_outdated_count)
    vmis=$(list_outdated_vmis)
    format_output "$count" "$vmis"
  fi
}

# Run
main
