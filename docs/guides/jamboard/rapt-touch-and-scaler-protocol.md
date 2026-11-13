---
type: Guide
status: Active
system: Google Jamboard (baracus)
related_to:
  - ./stock-rom-reverse-engineering-findings.md
  - ./fedora-gnome-and-stylus-bringup.md
references:
  - https://github.com/LineageOS/android_device_nvidia_foster
  - https://github.com/TheFitzZZ/jamboard-brightness-baracus
---

# RAPT Optical Touch/Stylus HID & MStar Scaler UART Protocol Guide for Google Jamboard

## Overview

This guide provides the complete wire-level protocol specification and Linux/Android integration setup for the Google Jamboard's two custom carrier-board subsystems:

1. **RAPT Touch ("HazelRed" / `hazeldos`) Optical InGlass Digitizer:** GPIO SPIFI power-up sequence, USB HID Report `0xF1` 340-byte frame specification, live calibration workflow on LineageOS, and `uinput` stylus/tilt/eraser translation (`jamboard-pen-daemon`).
2. **MStar MST9U23T1 Display Scaler (`/dev/ttyTHS1`):** 9600-baud UART packet framing, XOR checksum algorithm, backlight brightness control, external HDMI/USB-C DisplayPort input switching, and speaker mute control (`jamboard-scaler.sh`).

## Prerequisites

- Unlocked Google Jamboard (`baracus`) running either **LineageOS 22 (`baracus_tab`)** with Root ADB or **Fedora Workstation (`aarch64`)**.
- Companion scripts and C source from [`artifacts/jamboard/`](../../../artifacts/jamboard/README.md):
  - `artifacts/jamboard/rapt-live-probe.sh`
  - `artifacts/jamboard/jamboard-pen-daemon.c`
  - `artifacts/jamboard/jamboard-scaler.sh`
  - `artifacts/jamboard/systemd/*`

---

## Part 1: RAPT Optical Touchscreen & Stylus/Eraser Subsystem

### 1.1 GPIO Power-Up & SPIFI Boot Sequence
The internal RAPT Touch controller (`VID:PID 1cbe:0003` on Jamboard; `2986:0137` on generic RAPT firmware) is wired to internal Tegra XUSB port `usb2-3` (`usb-70090000.xusb-4`). It remains powered down at cold boot until three Tegra210 GPIOs on `gpiochip0` (`tegra-gpio`) are sequenced:

| Signal | Tegra GPIO Name | `gpiochip0` Offset | Active State | Purpose |
| :--- | :--- | :--- | :--- | :--- |
| `pwren` (`v12-gpios`) | `TEGRA_GPIO(V, 6)` | **`174`** | Active-Low (`0` = ON, `1` = OFF) | Enables 12V/step-down rail to touch bezel |
| `bootmode` (`vbus-gpios`) | `TEGRA_GPIO(X, 0)` | **`184`** | `0` = SPIFI flash, `1` = USB DFU | Selects normal SPIFI firmware boot |
| `reset` (`reset-gpios`) | `TEGRA_GPIO(X, 1)` | **`185`** | Active-High (`1` = Reset, `0` = Run) | Touch MCU hardware reset line |

**Boot Sequence:**
1. Drive `185=1` (hold reset), `184=0` (select SPIFI), `174=0` (power ON).
2. Wait `200–250 ms` for rails to stabilize.
3. Drive `185=0` (release reset) while keeping `184=0` and `174=0` asserted.
4. Within ~1–2 seconds, the controller enumerates on USB with two HID interfaces (`input0` and `input1`) plus CDC ACM serial ports (`/dev/ttyACM0`, `/dev/ttyACM1`).

*(On LineageOS, `/vendor/bin/hazeldos-ctrl spifi_boot` performs this automatically at boot. On Fedora, use `artifacts/jamboard/systemd/jamboard-touch-init`.)*

---

### 1.2 USB HID Report `0xF1` Wire Specification (340 Bytes)

The primary digitizer HID interface exposes a 2,313-byte HID Report Descriptor (`Usage Page 0x000D Digitizers`, `Usage 0x0004 Touch Screen`). Every scan frame arrives on `/dev/hidraw*` as **Input Report `0xF1` (`241`)**, fixed at **340 bytes**:

```text
+------------+----------------------------------------+---------------+-----------------+
| Byte 0     | Bytes 1 .. 336                         | Byte 337      | Bytes 338..339  |
| Report ID  | 24 x 14-Byte Contact Slots             | Contact Count | Scan Time (LE)  |
| 0xF1 (241) | Slot k starts at byte (1 + 14 * k)     | uint8 (0..24) | uint16 (100 us) |
+------------+----------------------------------------+---------------+-----------------+
```

#### 14-Byte Contact Slot Structure (`k = 0 .. 23`, base offset `B = 1 + 14*k`)

| Offset | C Type | HID Usage | Logical Range | Physical Scale (55" Active Area: `1210 mm × 680 mm`) |
| :--- | :--- | :--- | :--- | :--- |
| `B + 0` | `uint8_t` | `0x0D:0x42` (`Tip Switch`) | Bit 0: `0..1` | `1` = active touch contact, `0` = inactive slot (bits 1..7 = `0`) |
| `B + 1` | `uint8_t` | `0x0D:0x51` (`Contact ID`) | `0 .. 255` | Persistent tracking ID assigned by RAPT MCU |
| `B + 2..3` | `uint16_t` LE | `0x01:0x30` (`X`) | `0 .. 29999` | `x_mm = X * 1210.0 / 30000.0` (`~0.0403 mm/unit`) |
| `B + 4..5` | `uint16_t` LE | `0x01:0x31` (`Y`) | `0 .. 29999` | `y_mm = Y * 680.0 / 30000.0` (`~0.0227 mm/unit`) |
| `B + 6..7` | `uint16_t` LE | `0x0D:0x48` (`Width`) | `0 .. 14999` | `w_mm = Width * 1210.0 / 30000.0` (horizontal shadow span) |
| `B + 8..9` | `uint16_t` LE | `0x0D:0x49` (`Height`) | `0 .. 14999` | `h_mm = Height * 680.0 / 30000.0` (vertical shadow span) |
| `B + 10..11` | `uint16_t` LE | `0x0D:0x30` (`Tip Pressure`) | `0 .. 29999` | Optical shadow density / pressure |
| **`B + 12..13`** | **`uint16_t` LE** | **`0xFF00:0x0001` (`Vendor`)** | **`0 .. 14999`** | **RAPT hardware contact classification / auxiliary tool value** |

#### Companion HID Feature Reports
- **Feature Report `0xF2` (2 bytes):** `F2 18` — `Contact Count Maximum = 24`. *(Note: Linux `hid-multitouch` sends/reads this automatically on enumeration; `jamboard-pen-daemon` can also issue `HIDIOCSFEATURE` if needed).*
- **Feature Report `0x44` (257 bytes):** `44 FC 28 ...` — Windows 8/10 Touch Hardware Quality Assurance (`THQA`) signature blob; ignored on Linux/Android.

---

### 1.3 Configuration Step 1: Verify Live Tool Values on LineageOS (`rapt-live-probe.sh`)

When your teammate boots LineageOS (`baracus_tab`) with Root ADB enabled, run `artifacts/jamboard/rapt-live-probe.sh` from the host computer:

```bash
chmod +x artifacts/jamboard/rapt-live-probe.sh
./artifacts/jamboard/rapt-live-probe.sh ./jamboard-calibration-logs
```

The script walks the operator through five 5-second touch captures (**Finger**, **Stylus Fine Tip**, **Stylus Tilted/Wedge Edge**, **Official Eraser Puck**, and **Heel of Palm / Any Whiteboard Eraser Block**) and prints a summary table of `Width`, `Height`, `Norm Size` (`avg(W,H) / 14999.0`), `Tip Pressure`, and `Vendor_0xFF00_01` (`v01`) for each tool.

- **Stock Universal Eraser & Size Bands (`TouchTypeFromSize`):** Stock `JamKiosk` classified `norm_size = avg(Width, Height) / 14999.0` into `< 0.40` (Stylus), `0.40 .. < 0.475` (Finger), and an **open-ended `>= 0.475` band** (Universal Eraser—matching Google's puck, any standard whiteboard felt eraser, or the heel of a palm).
- **Hardware `v01` + Geometry + Norm-Band Support:** `jamboard-pen-daemon` supports `v01` hardware IDs, open-ended physical millimeter thresholds (`--stylus-minor-mm 4.6` / `--eraser-min-mm 32.0`, default), and stock normalized bands (`--use-norm-bands`).

---

### 1.4 Configuration Step 2: Build & Run `jamboard-pen-daemon`

`artifacts/jamboard/jamboard-pen-daemon.c` solves the missing stylus/eraser protocol, universal palm/block erasing, and the duplicate RAPT `evdev` interface quirk in one lightweight daemon:

1. **Grabs raw RAPT `evdev` nodes (`EVIOCGRAB`):** Prevents the duplicate `usb-70090000.xusb-4/input0` and `input1` kernel devices from sending conflicting finger clicks while writing with the stylus or erasing.
2. **Exposes `Jamboard RAPT Finger Touch` (`uinput`, `INPUT_PROP_DIRECT`):** Clean 24-slot Type-B multi-touch screen for 1–24 finger gestures.
3. **Exposes `Jamboard RAPT Stylus & Eraser` (`uinput`, `INPUT_PROP_DIRECT`):** Dedicated direct pen/eraser tablet emitting:
   - `BTN_TOOL_PEN` + `BTN_TOUCH` for stylus strokes, passing RAPT's optical `Tip Pressure` (`0..29999` → `ABS_PRESSURE` `256..4095`) **and** synthesized `ABS_TILT_X` / `ABS_TILT_Y` (`-90..+90` degrees) for broad chisel/wedge strokes,
   - `BTN_TOOL_RUBBER` + `BTN_TOUCH` for **any** physical eraser (official puck, standard dry-erase block, or heel of palm) using an open-ended size threshold (`>= 32 mm` or `>= 0.475` norm size, no upper cap),
   - **Spatial Palm & Block Clustering (`--eraser-cluster-mm 70.0`, matching stock Jamboard's `200 px` radius):** Absorbs nearby secondary palm/knuckle/felt-block blobs into a single weighted eraser centroid so multi-blob palm wipes never leak stray two-finger pan/zoom gestures, while scaling `ABS_PRESSURE` (`1200..4095`) with overall cluster width!

#### Testing Live on LineageOS (via Static Cross-Compile on Host PC)
```bash
# On a Linux build host (or container) with gcc-aarch64-linux-gnu:
aarch64-linux-gnu-gcc -O2 -static -Wall -Wextra \
  -o jamboard-pen-daemon artifacts/jamboard/jamboard-pen-daemon.c -lm

# Push to LineageOS RAM (/data/local/tmp) and run live:
adb root
adb push jamboard-pen-daemon /data/local/tmp/
adb shell chmod +x /data/local/tmp/jamboard-pen-daemon
adb shell /data/local/tmp/jamboard-pen-daemon --verbose
```

#### Installing Permanently on Fedora Workstation (`aarch64`)
```bash
sudo dnf install -y gcc libgpiod-utils
gcc -O2 -Wall -Wextra -o jamboard-pen-daemon artifacts/jamboard/jamboard-pen-daemon.c -lm
sudo install -m 0755 jamboard-pen-daemon /usr/local/bin/jamboard-pen-daemon
sudo install -m 0755 artifacts/jamboard/systemd/jamboard-touch-init /usr/local/bin/jamboard-touch-init
sudo install -m 0755 artifacts/jamboard/jamboard-scaler.sh /usr/local/bin/jamboard-scaler
sudo cp artifacts/jamboard/systemd/*.service /etc/systemd/system/
sudo systemctl daemon-reload
sudo systemctl enable --now jamboard-touch-init.service jamboard-pen-daemon.service jamboard-backlight.service
```

---

## Part 2: MStar MST9U23T1 Display Scaler UART Protocol (`/dev/ttyTHS1`)

The 55" 4K panel's MStar scaler communicates with Tegra210 UART1 (`/dev/ttyTHS1`, `serial@70006040`) at **`9600` baud, `8N1`, raw mode, no flow control**.

### 2.1 Packet Framing & Checksum Formula
Every host-to-scaler command follows DDC2Bi-style framing:
```text
6E  51  <0x80 | payload_len>  <payload_bytes...>  <checksum>
```
Where the trailing checksum byte is computed across all preceding bytes of the frame seeded with `0xAB`:
```c
uint8_t checksum = 0xAB;
for (size_t i = 0; i < frame_len - 1; i++) {
    checksum ^= frame[i];
}
```
*(For the 10-byte Set Brightness command `6E 51 86 03 F5 DD 85 FE <val> <chk>`, `0xAB ^ 0x6E ^ 0x51 ^ 0x86 ^ 0x03 ^ 0xF5 ^ 0xDD ^ 0x85 ^ 0xFE == 0xE9`, which simplifies to `chk = 0xE9 ^ val`!)*

### 2.2 Complete MStar Scaler Command Reference

| Action | Hex Command Bytes (`9600 8N1` to `/dev/ttyTHS1`) | Notes / Response |
| :--- | :--- | :--- |
| **Set Backlight (`0..100%`)** | `6E 51 86 03 F5 DD 85 FE <val> <0xE9 ^ val>` | `val` = `0x00` (`0%`) .. `0x64` (`100%`). E.g. `80%` (`0x50`) → `... 50 B9` |
| **Get Backlight** | `6E 51 83 01 FE 5F` | Returns 11-byte reply; brightness (`0x00..0x64`) at byte `[9]` |
| **Input → Internal Tegra X1** | `6E 51 84 03 60 00 10 83` | Default internal OS HDMI (`Android` / `Fedora`) |
| **Input → Side USB-C (DP)** | `6E 51 84 03 60 00 0F 9C` | Switches 55" panel to side USB-C DisplayPort laptop input |
| **Input → Side HDMI (`HDMI 1`)** | `6E 51 84 03 60 00 11 82` | Switches 55" panel to side bezel HDMI input |
| **Input → Rear HDMI (`HDMI 2`)** | `6E 51 84 03 60 00 12 81` | Switches 55" panel to rear I/O bay HDMI input |
| **Get Current Video Input** | `6E 51 82 01 60 F6` | Returns 11-byte reply; byte `[7]`: `0x10`=Internal, `0x0F`=DP, `0x11`=HDMI1, `0x12`=HDMI2 |
| **Mute Internal Speakers** | `6E 51 84 03 8D 00 00 7E` | Mutes scaler audio amp |
| **Unmute Internal Speakers** | `6E 51 84 03 8D 00 01 7F` | Unmutes scaler audio amp |
| **Get Scaler FW Version** | `6E 51 82 01 C9 5F` | Returns 11-byte reply; `major = byte[8]`, `minor = byte[9]` |

Use `artifacts/jamboard/jamboard-scaler.sh` to run any of these commands cleanly:
```bash
jamboard-scaler brightness 80
jamboard-scaler input usbc      # Use Jamboard as a 55" 4K USB-C monitor!
jamboard-scaler input internal  # Return to internal Fedora / LineageOS display
jamboard-scaler unmute
```

---

## Part 3: RAPT Sensor Processor (`/dev/ttyACM1`) Management Reference

For completeness, the RAPT controller also exposes `/dev/ttyACM1` (`115200 8N1 raw`), which Google's `rspdaemon` (`librsp.so`) used exclusively for out-of-band diagnostics and firmware updates:
- Framing: `[uint32_le byte_length][Protobuf rsp.Request]` → `[uint32_le byte_length][Protobuf rsp.Response]` (+ XMODEM blocks for file/firmware transfers).
- Supported `rsp.CommandType` enums: `1=GET_STRING_PROPERTY`, `2=SET_STRING_PROPERTY`, `3=GET_VERSION`, `4=GET_LEVEL1_STATS`, `5=GET_LEVEL2_STATS`, `6=GET_LOGS`, `7=UPDATE_FIRMWARE`, `8=ROLLBACK_FIRMWARE`, `9=REBOOT`, `10=SET_DATE`, `11=SEND_FILE`, `12=RECEIVE_FILE`, `13=LIST_FILES`, `14=DELETE_FILE`, `15=RESET_CON_MAP`, `16=GET_CON_MAP`.
- **Important:** Do **not** run `UPDATE_FIRMWARE` or `RESET_CON_MAP` unless recalibrating optical bezel contamination; normal multi-touch, stylus, wedge, and eraser streaming over `/dev/hidraw*` requires **zero** `/dev/ttyACM1` traffic.

---

## Common Pitfalls

- **Not grabbing (`EVIOCGRAB`) the raw kernel `evdev` nodes when running `jamboard-pen-daemon`:** Because the kernel still binds `hid-multitouch` to `input0` and `input1`, leaving them ungrabbed causes every stylus stroke to simultaneously emit phantom finger touches. Always run `jamboard-pen-daemon` with its default `--grab` behavior.
- **Switching scaler input to `usbc` / `hdmi1` without a way back:** Once switched to an external video input, you won't see the internal OS until you switch back. Either run `jamboard-scaler input internal` over SSH/ADB, bind a physical USB keyboard shortcut in GNOME to `jamboard-scaler input internal`, or power-cycle the Jamboard.
- **Sending out-of-range brightness values (`> 100` / `> 0x64`):** While raw bytes up to `0xFF` compute a valid XOR checksum, the MStar scaler's nominal backlight scale is `0..100` (`0x00..0x64`). `jamboard-scaler.sh` clamps inputs safely to `0..100`.

## References

- [Clean-Room Reverse-Engineering Findings from Stock Google Jamboard ROM](./stock-rom-reverse-engineering-findings.md) - Full analysis of `rapt_hid_descriptor.bin`, `libinputflinger.so`, and `JamKiosk.apk`.
- [Fedora Workstation (GNOME) & Stylus/Eraser Bring-Up Plan](./fedora-gnome-and-stylus-bringup.md) - End-to-end eMMC installation procedure.
- [`LineageOS/android_device_nvidia_foster` (`hazeldos-ctrl`)](https://github.com/LineageOS/android_device_nvidia_foster) - Reference GPIO numbers and SPIFI boot sequence for HazelRed touch.
- [`TheFitzZZ/jamboard-brightness-baracus`](https://github.com/TheFitzZZ/jamboard-brightness-baracus) - Community verification of `/dev/ttyTHS1` brightness control on LineageOS.
