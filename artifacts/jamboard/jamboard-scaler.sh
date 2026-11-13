#!/usr/bin/env bash
# ==============================================================================
# jamboard-scaler.sh — Google Jamboard MStar MST9U23T1 Scaler UART Utility
# ==============================================================================
# Purpose:
#   Controls the Google Jamboard's MStar MST9U23T1 4K display scaler over
#   Tegra210 UART1 (/dev/ttyTHS1 @ 9600 8N1 raw). Supports backlight brightness,
#   video input source switching (Internal OS, Side USB-C DisplayPort, Side
#   HDMI 1, Rear HDMI 2), speaker mute/unmute, and scaler firmware queries.
#
# Framing & Checksum (clean-room specification):
#   6E 51 <0x80 | len> <payload...> <checksum>
#   checksum = 0xAB ^ byte[0] ^ byte[1] ^ ... ^ byte[N-2]
#
# Usage:
#   jamboard-scaler.sh brightness <0-100>
#   jamboard-scaler.sh get-brightness
#   jamboard-scaler.sh input <internal|usbc|hdmi1|hdmi2>
#   jamboard-scaler.sh get-input
#   jamboard-scaler.sh mute | unmute
#   jamboard-scaler.sh version
#
# Environment overrides:
#   SCALER_TTY=/dev/ttyTHS1
# ==============================================================================

set -euo pipefail

TTY="${SCALER_TTY:-/dev/ttyTHS1}"

usage() {
  cat <<'EOF'
Usage: jamboard-scaler.sh <command> [args]

Commands:
  brightness <0-100>                  Set display backlight percentage (0..100)
  get-brightness                      Query current display backlight percentage
  input <internal|usbc|hdmi1|hdmi2>   Switch 55" 4K panel video source:
                                        internal (aliases: android, fedora, tx1)
                                        usbc     (aliases: dp, displayport)
                                        hdmi1    (alias: side-hdmi)
                                        hdmi2    (alias: rear-hdmi, back-hdmi)
  get-input                           Query active video input source
  mute                                Mute internal scaler audio amplifier
  unmute                              Unmute internal scaler audio amplifier
  version                             Query MStar scaler firmware version
EOF
}

require_tty() {
  if [[ ! -c "$TTY" ]]; then
    echo "ERROR: Character device $TTY not found (run as root on Jamboard)." >&2
    exit 1
  fi
  stty -F "$TTY" 9600 cs8 -cstopb -parenb raw -echo -ixon -ixoff min 0 time 3
}

# Send space-separated hex bytes, automatically appending (0xAB ^ XOR) checksum
send_packet() {
  local chk=$((0xAB))
  local hex_esc=""
  local b val
  for b in "$@"; do
    val=$((16#$b))
    chk=$((chk ^ val))
    hex_esc+=$(printf '\\x%02x' "$val")
  done
  hex_esc+=$(printf '\\x%02x' "$chk")
  # shellcheck disable=SC2059
  printf "$hex_esc" > "$TTY"
}

# Send query packet and read up to 16 response bytes as space-separated hex
query_packet() {
  require_tty
  # Drain any stale UART RX bytes
  dd if="$TTY" bs=64 count=1 status=none >/dev/null 2>&1 || true
  send_packet "$@"
  sleep 0.08
  dd if="$TTY" bs=16 count=1 status=none 2>/dev/null | od -An -tx1 | tr -s ' \n' ' ' | sed 's/^ //;s/ $//'
}

cmd="${1:-}"
case "$cmd" in
  brightness|set-brightness)
    pct="${2:-}"
    if [[ -z "$pct" || ! "$pct" =~ ^[0-9]+$ ]] || (( pct < 0 || pct > 100 )); then
      echo "ERROR: brightness requires an integer from 0 to 100." >&2
      exit 1
    fi
    require_tty
    hex_pct=$(printf '%02X' "$pct")
    send_packet 6E 51 86 03 F5 DD 85 FE "$hex_pct"
    ;;

  get-brightness)
    resp=$(query_packet 6E 51 83 01 FE)
    read -r -a bytes <<< "$resp"
    if (( ${#bytes[@]} >= 10 )); then
      echo "$((16#${bytes[9]}))"
    else
      echo "ERROR: Unexpected scaler reply (${resp:-empty})" >&2
      exit 1
    fi
    ;;

  input|set-input)
    src="${2:-}"
    case "$src" in
      internal|android|fedora|tx1) code="10" ;;
      usbc|dp|displayport)         code="0F" ;;
      hdmi1|side-hdmi)             code="11" ;;
      hdmi2|rear-hdmi|back-hdmi)   code="12" ;;
      *)
        echo "ERROR: Unknown input '$src'. Expected: internal, usbc, hdmi1, hdmi2." >&2
        exit 1
        ;;
    esac
    require_tty
    send_packet 6E 51 84 03 60 00 "$code"
    ;;

  get-input)
    resp=$(query_packet 6E 51 82 01 60)
    read -r -a bytes <<< "$resp"
    if (( ${#bytes[@]} >= 8 )); then
      case "${bytes[7],,}" in
        10) echo "internal (Tegra X1 HDMI)" ;;
        0f) echo "usbc (Side DisplayPort)" ;;
        11) echo "hdmi1 (Side HDMI)" ;;
        12) echo "hdmi2 (Rear HDMI)" ;;
        *)  echo "unknown (0x${bytes[7]})" ;;
      esac
    else
      echo "ERROR: Unexpected scaler reply (${resp:-empty})" >&2
      exit 1
    fi
    ;;

  mute)
    require_tty
    send_packet 6E 51 84 03 8D 00 00
    ;;

  unmute)
    require_tty
    send_packet 6E 51 84 03 8D 00 01
    ;;

  version)
    resp=$(query_packet 6E 51 82 01 C9)
    read -r -a bytes <<< "$resp"
    if (( ${#bytes[@]} >= 10 )); then
      printf "%d.%d (raw: %s)\n" "$((16#${bytes[8]}))" "$((16#${bytes[9]}))" "$resp"
    else
      echo "ERROR: Unexpected scaler reply (${resp:-empty})" >&2
      exit 1
    fi
    ;;

  -h|--help|help|"")
    usage
    ;;

  *)
    echo "ERROR: Unknown command '$cmd'." >&2
    usage >&2
    exit 1
    ;;
esac
