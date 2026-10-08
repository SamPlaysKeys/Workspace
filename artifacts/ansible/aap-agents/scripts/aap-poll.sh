#!/usr/bin/env bash
# Poll an AAP job until it exits running state.
#
# Usage:
#   aap-poll.sh <job-id> [min-interval-seconds]
#
# Each heartbeat line:
#   HH:MM:SS  <status>  <elapsed>s  ev/s=<rate>  interval=<next>s
#     task: <current task>  [host counts]
#
# Stdout cache is refreshed on activity bursts and every 10 polls.
# A mini task-history summary is printed every 10 polls.
# Final tasks printed on job completion.
#
# Exits 0 on success, 1 on failure/cancelled.

set -euo pipefail

JOB_ID="${1:?Usage: aap-poll.sh <job-id> [min-interval-seconds]}"
MIN_INTERVAL="${2:-10}"
STDOUT_CACHE=~/.cache/aap/${JOB_ID}-stdout.txt
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Resolve aap-api helper: check PATH, then script dir, then ~/.config/aap/
AAP_API="${AAP_API:-$(command -v aap-api 2>/dev/null || true)}"
if [[ -z "$AAP_API" && -x "$SCRIPT_DIR/aap-api" ]]; then
  AAP_API="$SCRIPT_DIR/aap-api"
elif [[ -z "$AAP_API" && -x "$HOME/.config/aap/aap-api" ]]; then
  AAP_API="$HOME/.config/aap/aap-api"
fi
if [[ -z "$AAP_API" ]]; then
  echo "ERROR: aap-api not found in PATH, $SCRIPT_DIR, or ~/.config/aap/" >&2
  exit 1
fi

prev_event_count=0
prev_poll_time=$(date +%s)
interval=30
poll_count=0

_current_task() {
  [[ -f "$STDOUT_CACHE" ]] || { echo "no stdout yet"; return; }
  grep "^TASK \[" "$STDOUT_CACHE" 2>/dev/null | tail -1 \
    | sed 's/^TASK \[//; s/\] \**//' \
    | cut -c1-70
}

_refresh_stdout() {
  python3 "$SCRIPT_DIR/aap-scan-stdout.py" "$JOB_ID" --no-cache >/dev/null 2>&1 || true
}

_last_tasks() {
  [[ -f "$STDOUT_CACHE" ]] || return
  grep "^TASK \[" "$STDOUT_CACHE" 2>/dev/null | tail -3 \
    | sed 's/^TASK \[//; s/\] \**//' \
    | while read -r t; do echo "    $t"; done
}

while true; do
  now=$(date +%s)
  (( poll_count++ )) || true

  # Transient network blips / momentary AAP 5xx must not kill a multi-hour
  # poll outright. aap-api exits non-zero on curl failure or HTTP>=400 (it
  # runs under its own set -euo pipefail), which would otherwise propagate
  # through this command substitution and terminate the script — the same
  # failure class already fixed for fatal_count below (grep -c on 0 matches).
  # Skip this iteration and retry after a short backoff instead of exiting.
  if ! payload=$("$AAP_API" GET "/api/v2/jobs/${JOB_ID}/" 2>/dev/null); then
    echo "$(date -u +%H:%M:%S)  ⚠ transient error polling job status, retrying in ${MIN_INTERVAL}s" >&2
    sleep "$MIN_INTERVAL"
    continue
  fi
  status=$(echo "$payload"  | jq -r '.status')
  elapsed=$(echo "$payload" | jq -r '.elapsed | tostring')
  counts=$(echo "$payload"  | jq -r '
    (.host_status_counts // {})
    | to_entries | map("\(.key)=\(.value)") | join("  ")
  ')

  if ! event_payload=$("$AAP_API" GET \
    "/api/v2/jobs/${JOB_ID}/job_events/?page_size=1" 2>/dev/null); then
    echo "$(date -u +%H:%M:%S)  ⚠ transient error polling job events, retrying in ${MIN_INTERVAL}s" >&2
    sleep "$MIN_INTERVAL"
    continue
  fi
  event_count=$(echo "$event_payload" | jq -r '.count // 0')

  wall_delta=$(( now - prev_poll_time ))
  event_delta=$(( event_count - prev_event_count ))
  if [[ $wall_delta -gt 0 ]]; then
    rate_x100=$(( event_delta * 100 / wall_delta ))
  else
    rate_x100=0
  fi

  if   [[ $rate_x100 -gt 500 ]]; then next_interval=$MIN_INTERVAL
  elif [[ $rate_x100 -gt 100 ]]; then next_interval=30
  elif [[ $rate_x100 -gt  10 ]]; then next_interval=60
  else                                 next_interval=120
  fi
  (( next_interval < MIN_INTERVAL )) && next_interval=$MIN_INTERVAL

  # Refresh stdout on activity bursts or every 10 polls
  if [[ $rate_x100 -gt 100 ]] || (( poll_count % 10 == 0 )); then
    _refresh_stdout
  fi

  fatal_count=$(grep -c "^fatal:\|^FAILED!" "$STDOUT_CACHE" 2>/dev/null || true)
  fatal_count=${fatal_count:-0}
  fatal_str=""
  [ "${fatal_count:-0}" -gt 0 ] && fatal_str="  ⚠ fatals=${fatal_count}"

  printf '%s  %-12s  %ss  ev/s=%s  interval=%ss%s\n    task: %s  %s\n' \
    "$(date -u +%H:%M:%S)" "$status" "${elapsed%.*}" \
    "$(awk "BEGIN{printf \"%.1f\", $rate_x100/100}")" \
    "$next_interval" "$fatal_str" \
    "$(_current_task)" "$counts"

  # Every 10 polls: mini task-history summary
  if (( poll_count % 10 == 0 )); then
    echo "  -- last 3 tasks --"
    _last_tasks
    echo ""
  fi

  [[ "$status" != "running" ]] && break

  prev_event_count=$event_count
  prev_poll_time=$now
  sleep "$next_interval"
done

echo ""
echo "=== Job ${JOB_ID}: ${status} (${elapsed%.*}s) ==="
_refresh_stdout
echo "  -- final tasks --"
_last_tasks

[[ "$status" == "successful" ]]
