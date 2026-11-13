---
type: README-Note
---
# Google Jamboard (`baracus`) Hardware Support Artifacts

Clean-room diagnostic tools, Linux/Android `uinput` stylus/eraser driver, MStar display scaler CLI, and `systemd` units for running **LineageOS** and **Fedora Workstation (`aarch64`, GNOME Wayland)** on a Google Jamboard.

## Contents

```text
artifacts/jamboard/
├── README.md                    # Build, testing, and installation instructions
├── rapt-live-probe.sh           # Guided ADB / local RAPT HID Report 0xF1 capture tool (needs root)
├── jamboard-pen-daemon.c        # Zero-dependency C daemon (/dev/hidraw* -> uinput Finger + Pen/Eraser)
├── captures/                    # Live LineageOS HID descriptors + per-phase capture summaries
├── jamboard-scaler.sh           # MStar MST9U23T1 /dev/ttyTHS1 CLI (brightness, HDMI/USB-C input, mute)
└── systemd/
    ├── jamboard-touch-init          # Tegra210 GPIO 174/184/185 SPIFI boot sequence
    ├── jamboard-touch-init.service  # Early-boot touchscreen power-on service
    ├── jamboard-pen-daemon.service  # Persistent RAPT uinput translator service
    └── jamboard-backlight.service   # Sets internal video source & 80% brightness at boot
```

---

## 1. Stage 1 Testing on LineageOS (Over Root ADB)

### A. Capture Empirical Tool Measurements (`rapt-live-probe.sh`)
Run from your laptop with the Jamboard booted into LineageOS (`baracus_tab`). Enable **Settings > Developer options > Rooted debugging** (ADB only); an unprivileged `adb shell` is blocked by SELinux from reading `/dev/hidraw*`. The script checks for root, picks the extended 3149-byte HID interface automatically, and captures with `adb exec-out cat`:

```bash
chmod +x artifacts/jamboard/rapt-live-probe.sh
adb root
PROBE_SECS=12 ./artifacts/jamboard/rapt-live-probe.sh ./jamboard-probe-results
```

Follow the five prompts (**Finger**, **Stylus**, **Eraser or Whiteboard Block**, **Heel of Palm**, **Stylus + Resting Palm**; type `s` + Enter to skip one) to record CSVs and summary statistics (`Width`, `Height`, `Width+Height`, `Tip Pressure`, firmware class vs. `Width+Height` class, mismatch count).

### B. Live-Test `jamboard-pen-daemon` on LineageOS
`jamboard-pen-daemon` classifies each contact using the RAPT firmware's own class code (`Vendor 0xFF00:0x02` on the extended interface: `1` stylus, `2` finger, `3` eraser) with a `Width+Height` fallback (`< 18000` stylus, `< 27000` finger, otherwise eraser) for the basic interface. Erasers and palms get `70 mm` spatial clustering (`--eraser-cluster-mm 70.0`) so palm wipes never trigger accidental two-finger pan/zoom. See [live input capture findings](../../docs/guides/jamboard/live-input-capture-findings.md) for the measured data.

Cross-compile a static `aarch64` binary on Linux (or inside a Linux container on macOS) and run it directly from `/data/local/tmp`:

```bash
aarch64-linux-gnu-gcc -O2 -static -Wall -Wextra \
  -o jamboard-pen-daemon artifacts/jamboard/jamboard-pen-daemon.c -lm

adb root
adb push jamboard-pen-daemon artifacts/jamboard/jamboard-scaler.sh /data/local/tmp/
adb shell chmod +x /data/local/tmp/jamboard-pen-daemon /data/local/tmp/jamboard-scaler.sh

# Stream live Stylus / Eraser uinput events on LineageOS:
adb shell /data/local/tmp/jamboard-pen-daemon --verbose
```

---

## 2. Stage 2 Installation on Fedora Workstation (`aarch64`)

Once Fedora boots (or inside an `aarch64` `systemd-nspawn` chroot during image assembly):

```bash
sudo dnf install -y gcc libgpiod-utils

gcc -O2 -Wall -Wextra -o jamboard-pen-daemon artifacts/jamboard/jamboard-pen-daemon.c -lm

sudo install -m 0755 jamboard-pen-daemon /usr/local/bin/jamboard-pen-daemon
sudo install -m 0755 artifacts/jamboard/jamboard-scaler.sh /usr/local/bin/jamboard-scaler
sudo install -m 0755 artifacts/jamboard/systemd/jamboard-touch-init /usr/local/bin/jamboard-touch-init
sudo cp artifacts/jamboard/systemd/*.service /etc/systemd/system/

sudo systemctl daemon-reload
sudo systemctl enable --now \
  jamboard-touch-init.service \
  jamboard-pen-daemon.service \
  jamboard-backlight.service
```

### Display Scaler CLI Examples
```bash
sudo jamboard-scaler brightness 75     # Set 55" 4K panel brightness to 75%
sudo jamboard-scaler get-brightness    # Read current brightness (0..100)
sudo jamboard-scaler input usbc        # Switch panel to side USB-C DisplayPort laptop input
sudo jamboard-scaler input hdmi1       # Switch panel to side HDMI port
sudo jamboard-scaler input hdmi2       # Switch panel to rear HDMI port
sudo jamboard-scaler input internal    # Switch panel back to internal Fedora / Tegra X1
sudo jamboard-scaler unmute            # Unmute internal scaler speakers
```

---

## Related Guides

- [Fedora Workstation (GNOME) & Stylus/Eraser Bring-Up Plan](../../docs/guides/jamboard/fedora-gnome-and-stylus-bringup.md)
- [RAPT Optical Touch/Stylus HID & MStar Scaler UART Protocol Guide](../../docs/guides/jamboard/rapt-touch-and-scaler-protocol.md)
- [Clean-Room Reverse-Engineering Findings from Stock Google Jamboard ROM](../../docs/guides/jamboard/stock-rom-reverse-engineering-findings.md)
- [Live Input Capture Findings (LineageOS, RAPT firmware classes)](../../docs/guides/jamboard/live-input-capture-findings.md)
