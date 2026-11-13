#!/usr/bin/env bash
# ==============================================================================
# rapt-live-probe.sh — Google Jamboard (baracus) RAPT HID Report 0xF1 Calibrator
# ==============================================================================
# Purpose:
#   Streams and decodes raw 340-byte USB HID Input Reports (Report ID 0xF1) from
#   the Jamboard's RAPT Touch InGlass digitizer (VID:PID 1cbe:0003 / 2986:0137),
#   recording empirical Width, Height, Tip Pressure, and Vendor (0xFF00:0x0001)
#   values across four guided test phases:
#     1. Single Finger
#     2. Passive Stylus (upright fine tip)
#     3. Passive Stylus (tilted onto flat wedge / chisel shoulder)
#     4. Passive Microfiber Eraser puck
#
# Usage:
#   From host PC over Root ADB (LineageOS):
#     ./rapt-live-probe.sh [output_dir]
#
#   Directly on the Jamboard (Fedora / local root shell):
#     sudo ./rapt-live-probe.sh --local [output_dir]
#
# Prerequisites:
#   - Host mode: adb (with Root ADB enabled on LineageOS), python3 (on host)
#   - Local mode: root privileges, python3
# ==============================================================================

set -euo pipefail

MODE="adb"
if [[ "${1:-}" == "--local" ]]; then
  MODE="local"
  shift
fi

OUT_DIR="${1:-./rapt-probe-$(date +%Y%m%d-%H%M%S)}"
mkdir -p "$OUT_DIR"

run_target() {
  if [[ "$MODE" == "adb" ]]; then
    adb shell "$@"
  else
    bash -c "$*"
  fi
}

echo "==> Mode: $MODE | Output directory: $OUT_DIR"

if [[ "$MODE" == "adb" ]]; then
  echo "==> Ensuring ADB root access..."
  adb wait-for-device
  adb root >/dev/null 2>&1 || true
  sleep 1
fi

# Locate the primary RAPT Touch Screen hidraw device (Report Descriptor == 2313 bytes)
HIDRAW_DEV="$(run_target '
best=""
for sys in /sys/class/hidraw/hidraw*; do
  [ -e "$sys/device/uevent" ] || continue
  if grep -qiE "1CBE:0003|2986:0137|RAPT" "$sys/device/uevent" 2>/dev/null; then
    dev="/dev/$(basename "$sys")"
    sz=0
    if [ -f "$sys/device/report_descriptor" ]; then
      sz=$(wc -c < "$sys/device/report_descriptor" | tr -d " ")
    fi
    if [ "$sz" = "2313" ]; then
      echo "$dev"
      exit 0
    fi
    [ -z "$best" ] && best="$dev"
  fi
done
echo "$best"
' | tr -d '\r\n')"

if [[ -z "$HIDRAW_DEV" ]]; then
  echo "ERROR: Could not find RAPT hidraw device (1cbe:0003 / 2986:0137)." >&2
  echo "       Ensure hazeldos-ctrl spifi_boot / jamboard-touch-init has run." >&2
  exit 1
fi

echo "==> Found RAPT primary HID device: $HIDRAW_DEV"

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
  read -r -p "Press [Enter] when ready, then touch/draw continuously for ~5 seconds..." _

  echo "==> Capturing ~400 HID frames from $HIDRAW_DEV..."
  if [[ "$MODE" == "adb" ]]; then
    adb exec-out "dd if=$HIDRAW_DEV bs=340 count=400 2>/dev/null" > "$raw_bin"
  else
    dd if="$HIDRAW_DEV" bs=340 count=400 status=none > "$raw_bin"
  fi

  python3 - "$raw_bin" "$csv_out" "$phase_id" <<'PYEOF'
import struct
import sys
import statistics

raw_path, csv_path, phase = sys.argv[1], sys.argv[2], sys.argv[3]
data = open(raw_path, "rb").read()

# Resynchronize on 0xF1 frames (340 bytes each)
rows = []
i = 0
n = len(data)
while i + 340 <= n:
    if data[i] != 0xF1:
        nxt = data.find(b"\xf1", i + 1)
        if nxt == -1:
            break
        i = nxt
        continue
    frame = data[i : i + 340]
    i += 340
    contact_count = frame[337]
    scan_time = struct.unpack_from("<H", frame, 338)[0]
    for slot in range(24):
        base = 1 + slot * 14
        tip, cid, x, y, w, h, p, v01 = struct.unpack_from("<BBHHHHHH", frame, base)
        if tip & 0x01:
            w_mm = w * 1210.0 / 30000.0
            h_mm = h * 680.0 / 30000.0
            rows.append((scan_time, contact_count, slot, cid, x, y, w, h, round(w_mm, 2), round(h_mm, 2), p, v01))

with open(csv_path, "w") as f:
    f.write("scan_time,contact_count,slot,cid,x,y,width_raw,height_raw,width_mm,height_mm,pressure,vendor_0xff00_01\n")
    for r in rows:
        f.write(",".join(map(str, r)) + "\n")

if not rows:
    print(f"  [!] No active touch contacts captured in {phase}! Try pressing firmly while capturing.")
    sys.exit(0)

ws = [r[6] for r in rows]
hs = [r[7] for r in rows]
wmms = [r[8] for r in rows]
hmms = [r[9] for r in rows]
maj_mms = [max(r[8], r[9]) for r in rows]
min_mms = [min(r[8], r[9]) for r in rows]
ps = [r[10] for r in rows]
vs = sorted(set(r[11] for r in rows))

print(f"  Captured {len(rows)} active contact samples -> {csv_path}")
print(f"  Vendor 0xFF00:0x0001 distinct values : {vs}")
print(f"  Width  (raw / mm) : min={min(ws)} ({min(wmms):.1f}mm)  median={int(statistics.median(ws))} ({statistics.median(wmms):.1f}mm)  max={max(ws)} ({max(wmms):.1f}mm)")
print(f"  Height (raw / mm) : min={min(hs)} ({min(hmms):.1f}mm)  median={int(statistics.median(hs))} ({statistics.median(hmms):.1f}mm)  max={max(hs)} ({max(hmms):.1f}mm)")
print(f"  Major / Minor mm  : median_major={statistics.median(maj_mms):.1f}mm  median_minor={statistics.median(min_mms):.1f}mm")
print(f"  Tip Pressure      : min={min(ps)}  median={int(statistics.median(ps))}  max={max(ps)}")
PYEOF
}

capture_phase "01_finger"       "Draw circles on screen using ONE FINGER."
capture_phase "02_stylus_tip"   "Draw lines holding the PASSIVE STYLUS UPRIGHT (fine tip)."
capture_phase "03_stylus_wedge" "Draw broad strokes holding the PASSIVE STYLUS TILTED FLAT onto its chisel/wedge edge."
capture_phase "04_eraser"       "Wipe across the screen flat with the MICROFIBER ERASER PUCK."

echo ""
echo "==> Calibration probe complete! All raw dumps and CSVs saved in: $OUT_DIR"
