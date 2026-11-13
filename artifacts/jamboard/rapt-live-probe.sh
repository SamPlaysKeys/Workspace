#!/usr/bin/env bash
# ==============================================================================
# rapt-live-probe.sh — Google Jamboard (baracus) RAPT HID Report 0xF1 Calibrator
# ==============================================================================
# Purpose:
#   Captures and decodes the raw USB HID Input Reports (Report ID 0xF1) from the
#   Jamboard's RAPT Touch InGlass digitizer (VID:PID 2986:0137 / 1cbe:0003) and
#   summarises, per guided test phase:
#     - firmware contact class (Vendor 0xFF00:0x02: 1=stylus, 2=finger, 3=eraser)
#     - Width / Height / Width+Height (Width+Height is constant per class)
#     - Tip Pressure (optical contact area, not force)
#     - simultaneous contacts per frame (palm / eraser blob splitting)
#
#   The extended 3149-byte-descriptor interface is preferred: it carries the
#   firmware class (484-byte Report 0xF1 with 20-byte contacts) and also streams
#   103-byte Report 0xF6 (contamination map), which is skipped. The basic
#   2313-byte interface (340-byte Report 0xF1, 14-byte contacts) is used as a
#   fallback; classes are then derived from Width+Height.
#
# Phases:
#   01_finger            Single finger
#   02_stylus            Passive stylus (upright)
#   03_eraser_or_block   Any whiteboard eraser / felt block (or the Google puck)
#   04_palm              Heel of palm
#   05_pen_plus_palm     Stylus with palm resting on the glass
#
# Usage:
#   From host PC over Root ADB (LineageOS; enable Developer options > Rooted debugging):
#     ./rapt-live-probe.sh [output_dir]
#
#   Directly on the Jamboard (Fedora / local root shell):
#     sudo ./rapt-live-probe.sh --local [output_dir]
#
#   PROBE_SECS=20 ./rapt-live-probe.sh   # capture length per phase (default 12 s)
#
# Prerequisites:
#   - Host mode: adb (with Root ADB enabled), python3 (on host)
#   - Local mode: root privileges, python3
#   - Reading /dev/hidraw* requires root: SELinux blocks the unprivileged adb shell.
# ==============================================================================

set -euo pipefail

MODE="adb"
if [[ "${1:-}" == "--local" ]]; then
  MODE="local"
  shift
fi

OUT_DIR="${1:-./rapt-probe-$(date +%Y%m%d-%H%M%S)}"
SECS="${PROBE_SECS:-12}"
mkdir -p "$OUT_DIR"

run_target() {
  if [[ "$MODE" == "adb" ]]; then
    adb shell "$@"
  else
    bash -c "$*"
  fi
}

echo "==> Mode: $MODE | Output directory: $OUT_DIR | Capture length: ${SECS}s"

if [[ "$MODE" == "adb" ]]; then
  echo "==> Ensuring ADB root access..."
  adb wait-for-device
  adb root >/dev/null 2>&1 || true
  sleep 1
  if [[ "$(adb shell id -u | tr -d '\r\n')" != "0" ]]; then
    echo "ERROR: adb is not root. Enable Settings > Developer options > Rooted debugging." >&2
    exit 1
  fi
else
  if [[ "$(id -u)" != "0" ]]; then
    echo "ERROR: --local mode requires root (sudo)." >&2
    exit 1
  fi
fi

# Pick the hidraw node: extended (3149-byte rdesc) preferred, else basic (2313)
PICK="$(run_target '
ext=""; basic=""
for sys in /sys/class/hidraw/hidraw*; do
  [ -e "$sys/device/uevent" ] || continue
  grep -qiE "0003:00002986:00000137|0003:00001CBE:00000003" "$sys/device/uevent" 2>/dev/null || continue
  dev="/dev/$(basename "$sys")"
  sz=0
  [ -f "$sys/device/report_descriptor" ] && sz=$(wc -c < "$sys/device/report_descriptor" | tr -d " ")
  [ "$sz" = "3149" ] && ext="$dev"
  [ "$sz" = "2313" ] && basic="$dev"
done
if [ -n "$ext" ]; then echo "extended $ext"; elif [ -n "$basic" ]; then echo "basic $basic"; fi
' | tr -d '\r')"

if [[ -z "$PICK" ]]; then
  echo "ERROR: Could not find a RAPT hidraw device (2986:0137 / 1cbe:0003)." >&2
  echo "       Ensure hazeldos-ctrl spifi_boot / jamboard-touch-init has run." >&2
  exit 1
fi

LAYOUT="${PICK%% *}"
HIDRAW_DEV="${PICK##* }"
echo "==> Using RAPT $LAYOUT interface: $HIDRAW_DEV"

capture_phase() {
  local phase_id="$1"
  local prompt_msg="$2"
  local raw_bin="$OUT_DIR/${phase_id}.raw"
  local csv_out="$OUT_DIR/${phase_id}.csv"

  echo ""
  echo "----------------------------------------------------------------------"
  echo "PHASE: $phase_id"
  echo "INSTRUCTION: $prompt_msg"
  echo "----------------------------------------------------------------------"
  read -r -p "Press [Enter] to start, then touch/draw continuously for ${SECS} seconds (type 's' + Enter to skip)..." ans
  if [[ "${ans:-}" == "s" ]]; then
    echo "  (skipped)"
    return 0
  fi

  echo "==> Capturing ${SECS}s from $HIDRAW_DEV..."
  if [[ "$MODE" == "adb" ]]; then
    adb exec-out "timeout $SECS cat $HIDRAW_DEV" > "$raw_bin" || true
  else
    timeout "$SECS" cat "$HIDRAW_DEV" > "$raw_bin" || true
  fi

  python3 - "$raw_bin" "$csv_out" "$phase_id" "$LAYOUT" <<'PYEOF'
import collections
import statistics
import struct
import sys

raw_path, csv_path, phase, layout = sys.argv[1:5]
data = open(raw_path, "rb").read()

if layout == "extended":
    lens, clen, has_class = {0xF1: 484, 0xF6: 103}, 20, True
else:
    lens, clen, has_class = {0xF1: 340}, 14, False
CC_OFF = 1 + 24 * clen
SCAN_OFF = CC_OFF + 1

# Frame walk: each report starts with its Report ID; unknown bytes are skipped
rows, frames, f6, i = [], 0, 0, 0
while i < len(data):
    n = lens.get(data[i])
    if n is None:
        i += 1
        continue
    if i + n > len(data):
        break
    frame = data[i:i + n]
    i += n
    if frame[0] == 0xF6:
        f6 += 1
        continue
    frames += 1
    cc = frame[CC_OFF]
    scan = struct.unpack_from("<H", frame, SCAN_OFF)[0]
    for slot in range(24):
        b = 1 + slot * clen
        tip = frame[b] & 1
        if not tip:
            continue
        fw = frame[b] >> 1 if has_class else 0
        cid = frame[b + 1]
        x, y, w, h, p = struct.unpack_from("<HHHHH", frame, b + 2)
        total = w + h
        sum_cls = 1 if total < 18000 else (2 if total < 27000 else 3)
        rows.append((scan, cc, slot, cid, fw, sum_cls, x, y, w, h, total, p,
                     round(x * 1207.6 / 30000.0, 1), round(y * 678.4 / 30000.0, 1)))

with open(csv_path, "w") as fh:
    fh.write("scan_time,contact_count,slot,cid,fw_class,sum_class,x,y,width,height,w_plus_h,tip_pressure,x_mm,y_mm\n")
    for r in rows:
        fh.write(",".join(map(str, r)) + "\n")

print(f"  F1 frames: {frames}   F6 (contamination map) frames: {f6}   contact rows: {len(rows)} -> {csv_path}")
if not rows:
    print(f"  [!] No active contacts captured in {phase}. Keep touching the glass during capture.")
    sys.exit(0)


def stat(name, vals):
    print(f"  {name:<22}: min={min(vals)}  median={int(statistics.median(vals))}  max={max(vals)}")


stat("simultaneous contacts", [r[1] for r in rows])
stat("Width", [r[8] for r in rows])
stat("Height", [r[9] for r in rows])
stat("Width+Height", [r[10] for r in rows])
stat("Tip Pressure", [r[11] for r in rows])
names = {0: "n/a", 1: "stylus", 2: "finger", 3: "eraser"}
if has_class:
    fw_hist = collections.Counter(r[4] for r in rows)
    print("  firmware class        : " + ", ".join(f"{names.get(k, k)}={v}" for k, v in sorted(fw_hist.items())))
sum_hist = collections.Counter(r[5] for r in rows)
print("  Width+Height class    : " + ", ".join(f"{names[k]}={v}" for k, v in sorted(sum_hist.items())))
if has_class:
    mism = sum(1 for r in rows if r[4] != r[5])
    print(f"  firmware vs W+H class mismatches: {mism} / {len(rows)}")
PYEOF
}

capture_phase "01_finger"          "Draw circles on screen using ONE FINGER."
capture_phase "02_stylus"          "Draw lines with the PASSIVE STYLUS held upright."
capture_phase "03_eraser_or_block" "Wipe across the screen flat with ANY WHITEBOARD ERASER / felt block (or the Google puck)."
capture_phase "04_palm"            "Wipe across the screen using the HEEL OF YOUR PALM."
capture_phase "05_pen_plus_palm"   "Draw with the STYLUS while RESTING YOUR PALM on the glass."

echo ""
echo "==> Calibration probe complete! Raw dumps and CSVs saved in: $OUT_DIR"
