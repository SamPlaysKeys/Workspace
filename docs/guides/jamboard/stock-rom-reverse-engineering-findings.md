---
type: Guide
status: Active
system: Google Jamboard (baracus)
related_to:
  - ./fedora-gnome-and-stylus-bringup.md
  - ./rapt-touch-and-scaler-protocol.md
references:
  - https://github.com/LineageOS/android_device_google_baracus
  - https://github.com/LineageOS/android_kernel_nvidia_linux-4.9
  - https://github.com/TheFitzZZ/jamboard-brightness-baracus
---

# Clean-Room Reverse-Engineering Findings from Stock Google Jamboard ROM (`NRT1.240624.004`)

## Overview

This document records the clean-room interoperability findings obtained from inspecting partition backups (`stock-system.img`, `stock-vendor.img`, and `stock-wifi-rp3.img`) pulled from a Google Jamboard (`baracus`) running Google's final End-of-Life Android 7.0 release (`google/baracus/baracus:7.0/NRT1.240624.004/12020658:user/release-keys`).

The primary goal of this investigation was to determine how Google's stock OS distinguished between **fingers**, the **passive stylus (fine tip)**, **tilted/broad-edge stylus strokes (wedge)**, and the **passive microfiber eraser**, and why unmodified AOSP/LineageOS and mainline Linux kernels only expose basic finger touch.

> **Clean-Room & Interoperability Scope:** This document contains only factual hardware interface specifications, USB HID descriptor structures, UART command framing, and functional descriptions required to build independent open-source Linux/Android drivers for user-owned EoL hardware. No proprietary binaries, firmware blobs, or copyrighted source code are reproduced.

---

## 1. Partition & Firmware Inventory Summary

| Partition Image | Filesystem / Size | Key Interoperability Findings |
| :--- | :--- | :--- |
| `stock-system.img` (`APP`) | `ext4`, 2.0 GiB | Contains `/system/etc/rapt_hid_descriptor.bin` (2,313 B), Google's `karma` hardware daemons (`touchdaemon`, `rspdaemon`, `scalerdaemon`, `karmausbdaemon`), custom `libinputflinger.so` tool-type mapping, `JamKiosk.apk` (`libjamkiosk.so` stroke/wedge math), and `nvram_jetsonE_cv_4354.txt`. |
| `stock-vendor.img` (`vendor`) | `ext4`, 768 MiB | Contains Broadcom Wi-Fi firmware (`fw_bcmdhd.bin`), NXP NFC firmware (`libpn547_fw.so`), Tegra audio DSP (`adsp.elf`), and `libjamboardaec.so`. *(Note: `rm_ts_server` / `librm31080.so` are unused NVIDIA Shield Raydium leftovers).* |
| `stock-wifi-rp3.img` (`RP3`) | `ext4`, 8.0 MiB | **Empty** (contains only `lost+found`). Confirms Wi-Fi (`bcm4354`) operates directly from the generic board NVRAM template `/system/etc/nvram_jetsonE_cv_4354.txt` without unit-specific `RP3` files on this revision. |

### NVIDIA Tegra Boilerplate Ruled Out
Several files in `/system/etc/` look touch-related at first glance but were confirmed to be unused NVIDIA Shield (`foster`/`loki`) reference artifacts:
- `/system/etc/freeMotionD.config` — Audience ICM20268 6-axis IMU configuration for Shield controllers.
- `/system/etc/calib_cfg.xml` & `/vendor/bin/raydium_selftest` — NVIDIA Raydium capacitive touch test configs.
- `/system/bin/update_js_touch_fw.sh` — GreenPeak/Cypress Shield controller updater.
- `/system/usr/idc/touch.idc` — Generic Atmel maXTouch IDC (`touch.toolSize.calibration = area`, `touch.orientation.calibration = vector`).

---

## 2. The Smoking Gun: Why Stylus & Eraser Fail on Unpatched Linux / LineageOS

Tracing the touch data path from the USB wire all the way to the whiteboard renderer revealed three connected architectural facts:

### Finding A: RAPT Hardware Classifies Contacts in a Custom 16-Bit HID Vendor Field
`/system/etc/rapt_hid_descriptor.bin` (2,313 bytes) defines the exact USB HID Report Descriptor for the RAPT Touch InGlass controller (`VID:PID 1cbe:0003`).
- Primary touch streaming uses **Input Report `0xF1` (`241`)**, which is **340 bytes** long and carries **24 fixed 14-byte contact slots** plus `Contact Count` (1 B) and `Scan Time` (2 B).
- Every contact slot is wrapped inside a standard `Logical Collection (Finger, 0x000D:0x0022)` containing `Tip Switch`, `Contact ID`, `X`, `Y`, `Width` (`0x0D:0x48`), `Height` (`0x0D:0x49`), `Tip Pressure` (`0x0D:0x30`), and—at contact bytes `+12..+13`—a **16-bit vendor-defined field (`Usage Page 0xFF00, Usage 0x01`, range `0..14999`)**.
- Because `touchdaemon` simply forwards raw `/dev/hidraw*` reports to `/dev/hidg0` using `rapt_hid_descriptor.bin` and no user-space daemon injects `uinput` events on stock Android, **all contact geometry and hardware tool classification already arrive over USB in Input Report `0xF1`**.

### Finding B: Unpatched `hid-multitouch.c` Drops `0xFF00:0x0001` and Hardcodes `MT_TOOL_FINGER`
Comparing `rapt_hid_descriptor.bin` against upstream Linux and `LineageOS/android_kernel_nvidia_linux-4.9` (`drivers/hid/hid-multitouch.c`):
1. `hid-multitouch` has no quirk or handler for `0x1cbe:0x0003` or `0xFF00:0x0001`, so `mt_touch_input_mapping()` ignores bytes `+12..+13` (`return 0`).
2. Because each slot's enclosing HID collection is `HID_DG_FINGER` (`0x000D:0x0022`), `mt_process_slot()` unconditionally reports `ABS_MT_TOOL_TYPE = MT_TOOL_FINGER (0)` for every contact—whether finger, passive stylus, or physical eraser.

### Finding C: Stock `libinputflinger.so` Added a Custom `MT_TOOL_ERASER = 3` Mapping
Inspecting `/system/lib64/libinputflinger.so` showed that Google customized Android's `InputFlinger` specifically for the Jamboard's kernel driver:
- In standard AOSP, `ABS_MT_TOOL_TYPE` only defines `0` (`MT_TOOL_FINGER`), `1` (`MT_TOOL_PEN`), and `2` (`MT_TOOL_PALM`). Standard AOSP cannot produce `AMOTION_EVENT_TOOL_TYPE_ERASER (4)` from `ABS_MT_TOOL_TYPE` alone without `BTN_TOOL_RUBBER`.
- In Jamboard's stock `libinputflinger.so` (`MultiTouchInputMapper::syncTouch` @ `0x4d9e0`, lookup table `@ 0x55e40`), Google extended the `ABS_MT_TOOL_TYPE` translation table to **4 entries (`0..3`)**:
  - `0` → `1` (`AMOTION_EVENT_TOOL_TYPE_FINGER`)
  - `1` → `2` (`AMOTION_EVENT_TOOL_TYPE_STYLUS`)
  - `2` → `0` (`AMOTION_EVENT_TOOL_TYPE_UNKNOWN`, falling back to `BTN_TOOL_*`)
  - **`3` → `4` (`AMOTION_EVENT_TOOL_TYPE_ERASER`)**
- Stock `libinputflinger.so` also includes spatial/temporal eraser latching (`TouchInputMapper::trackToolSwitch` / `NVPreProcTouchInput`, controlled by system properties `input.ptr_trk.eraserlatch_range` and `input.ptr_trk.eraserdelay_time`) to prevent a lifted or bouncing eraser puck from briefly flickering to finger/stylus tool types within a bounding box.

**Implication for LineageOS & Fedora GNOME:**
Rather than patching custom non-standard `ABS_MT_TOOL_TYPE = 3` values into both the kernel and Android/Wayland compositors, a clean user-space daemon (`jamboard-pen-daemon`) reading `/dev/hidraw*` can split fingers onto a multi-touch `uinput` screen device and emit stylus/eraser contacts onto a dedicated `uinput` direct tablet device using standard Linux `BTN_TOOL_PEN` and `BTN_TOOL_RUBBER`—which **both** unmodified LineageOS (`TouchButtonAccumulator`) and unmodified Fedora GNOME Wayland (`libinput` tablet tool) understand natively out of the box.

---

## 3. How Stock Jamboard Handled Stylus "Tilt" (Wedge / Chisel) & Eraser

Inspecting the Android `MotionEvent` -> JNI bridge and C++ stroke engine inside `JamKiosk.apk` (`PectinSurfaceView` touch translator and `libjamkiosk.so`) clarified exactly what optical axes matter:

### 1. Stock Jamboard Never Used `AXIS_TILT` (`ABS_TILT_X` / `ABS_TILT_Y`)
When translating an Android `MotionEvent` into internal touch points, the app reads only:
- `getPointerId(i)`
- `getToolType(i)` → mapped to `FINGER` (`1`), `STYLUS` (`2`), or `ERASER` (`4` → internal enum `5`)
- `getX(i)` and `getY(i)`
- **`getTouchMajor(i)` and `getTouchMinor(i)`**
- `getPressure(i)`
- **`getOrientation(i)`** (optional float angle in radians)

### 2. Passive Stylus Broad-Edge ("Tilt" / Chisel) Geometry
Because the Jamboard stylus is completely passive (an optical interruptor with a ~2 mm fine tip and a ~15–18 mm flat chisel shoulder), tilting the stylus onto its flat edge physically widens the infrared shadow on the glass into an elongated ellipse:
- In `PolylinePacker::AddPoint`, when drawing with `STYLUS`, the engine evaluates `major_px = max(touchMajor, touchMinor)` on the 3840×2160 canvas:
  - **`major_px <= 15.0 px` (~4.7 mm physical span):** Treated as a normal upright fine-tip pen stroke (base stroke width multiplier = `1.0`).
  - **`15.0 px < major_px <= 60.0 px` (~4.7 mm – 19.0 mm physical span):** Treated as an angled wedge/broad-edge stroke; stroke width expands linearly up to the tool's maximum ribbon width, and the ribbon normal vector rotates by `(sin(orientation), -cos(orientation))` to produce calligraphic/chisel strokes.

### 3. Hardware Puck Eraser vs. Finger Wipe Eraser
In `DrawingController::StartTouchGroupAtPoint`:
- Contacts arriving with `TOOL_TYPE_ERASER` immediately start a dedicated hardware eraser group (`CreateEraserTouchGroup`), rendering an eraser cursor sized to the puck's contact diameter.
- Contacts arriving with `TOOL_TYPE_FINGER` manipulate canvas objects unless the optional `FingerEraser` feature is enabled and `max(touchMajor, touchMinor)` exceeds ~`36 px` (~11.4 mm), allowing palm/multi-finger wiping.

### 4. Translating Optical Wedge into Standard `ABS_TILT_X` / `ABS_TILT_Y` for Linux & Android Apps
Desktop Linux whiteboard apps (RNote, Xournal++, Krita) and modern Android apps expect standard Wacom-style `ABS_TILT_X` / `ABS_TILT_Y` (`-90°..+90°`) on a `BTN_TOOL_PEN` device rather than `ABS_MT_TOUCH_MAJOR` width on a stylus.
From RAPT's physical `Width` (`w_mm`) and `Height` (`h_mm`):
- `major_mm = max(w_mm, h_mm)` and `minor_mm = min(w_mm, h_mm)`
- When `major_mm > 4.5 mm` on a stylus contact, the tilt magnitude (`0°..75°`) can be synthesized smoothly from either contact elongation `acos(clamp(minor_mm / major_mm, 0.25, 1.0))` or linear wedge span `(major_mm - 4.5) / (18.0 - 4.5)`, projected onto X/Y axes according to whether `w_mm >= h_mm`.

---

## 4. Roles of Google's Native `karma` Daemons in `/system/bin/`

| Binary & Library | IPC / Device Nodes | Functional Role & Linux Porting Impact |
| :--- | :--- | :--- |
| `scalerdaemon` (`libscalerdaemon.so`) | `/dev/ttyTHS1` (`9600 8N1`), `/dev/nvhdcp0`, Binder `karma.scaler.IScalerDaemon` | Controls the MStar MST9U23T1 display scaler: backlight brightness (`0..100`), video source selection (`Android`, `USB-C DP`, `Side HDMI 1`, `Rear HDMI 2`), speaker mute enforcement, and scaler FW version readout. **Needed on Linux** (replaced by lightweight `jamboard-scaler.sh`). |
| `touchdaemon` (`libtouchdaemon.so`) | `/dev/hidraw*` (`"RAPT"`), `/dev/hidg0`, Binder `karma.ITouchDaemon` | Listens to `scalerdaemon` input changes. When switched to external HDMI/USB-C laptop video, disables local Android touch (`IInputManager`) and copies raw 340-byte HID reports from `/dev/hidraw*` to USB gadget `/dev/hidg0` so the external laptop gets touch. **Not needed for local OS use** (unless building external monitor touch pass-through later). |
| `rspdaemon` (`librspdaemon.so`, `librsp.so`) | `/dev/ttyACM1` (`115200 8N1`), `/dev/hidraw*`, Binder `karma.rsp.IRspDaemon` | Speaks length-prefixed Protobuf (`rsp.Request` / `rsp.Response`) + XMODEM to the HazelRed MCU for diagnostics, firmware updates, logs, and optical bezel contamination maps (`GET_CON_MAP` / `RESET_CON_MAP`). **Not needed for normal touch/stylus operation.** |
| `karmausbdaemon` (`libkarmausbfilter.so`) | `libusb` host | Manages companion Google Meet conference USB peripherals (`atrus` speakerphone, `huddly` camera). **Not needed.** |

---

## Related Documentation & Artifacts

- [RAPT Touch HID & MStar Scaler Protocol Reference](./rapt-touch-and-scaler-protocol.md) - Exact byte layouts, UART packet tables, and `uinput` daemon architecture.
- [Fedora Workstation (GNOME) & Stylus/Eraser Bring-Up Plan](./fedora-gnome-and-stylus-bringup.md) - Full step-by-step LineageOS and internal-eMMC Fedora installation guide.
- [`artifacts/jamboard/`](../../../artifacts/jamboard/README.md) - Ready-to-run `rapt-live-probe.sh`, `jamboard-pen-daemon.c`, `jamboard-scaler.sh`, and systemd service units.

## References

- [`LineageOS/android_device_google_baracus`](https://github.com/LineageOS/android_device_google_baracus) - Open-source device tree, `hazeldos-ctrl` GPIO init sequence, and `input-port-associations.xml`.
- [`LineageOS/android_kernel_nvidia_linux-4.9`](https://github.com/LineageOS/android_kernel_nvidia_linux-4.9) - Downstream Tegra210 kernel confirming unpatched `drivers/hid/hid-multitouch.c` behavior.
