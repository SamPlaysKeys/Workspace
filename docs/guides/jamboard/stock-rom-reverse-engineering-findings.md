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

## 3. How Stock Jamboard Handled Stylus "Tilt" (Wedge / Chisel), Universal Erasers, & Palm Wiping

Inspecting the unobfuscated Protobuf schema `com.google.apps.pectin.android.proto.Touch$TouchPoint` in `classes2.dex` (`PectinContainer.transferTouchMotionEventToPectin`) alongside `libjamkiosk.so` revealed how Google supported **any** physical dry-erase block, the heel of a palm, and passive chisel-edge shading:

### 1. Stock `PectinContainer` Touch Schema (`Touch$TouchPoint`)
When `JamKiosk` translates an Android `MotionEvent` into its native C++ stroke engine (`pectin::platform::android::PectinContainer::Respond`), it populates only:
- `id_` (`field 1`): `MotionEvent.getPointerId(i)`
- `type_` (`field 2`): `BEGIN (1)`, `MOVE (2)`, `STAY (3)`, `END (4)`, `CANCEL (5)`
- `toolType_` (`field 3`): **`FINGER (1)` or `STYLUS (2)` only** (`MotionEvent.getToolType(i) == 2 ? STYLUS : FINGER` — `Touch$TouchPoint` has no `ERASER` enum value!)
- `x_` / `y_` / `lastX_` / `lastY_` (`fields 4..7`): `getX(i)`, `getY(i)`, and previous coordinates
- **`size_` (`field 8`): `MotionEvent.getSize(i)`** (computed by Android `TouchInputMapper::cookPointerData` as `0.5 * (rawTouchMajor + rawTouchMinor) / 14999.0`)
- `timestamp_` (`field 9`): `MotionEvent.getEventTime()`
- **`pressure_` (`field 10`): `MotionEvent.getPressure(i)`** (optional float from RAPT HID `Tip Pressure` `0x0D:0x30`)

*(Note: A generic `Lagsr;` compatibility helper in `classes14.dex` references `getTouchMajor` and `getOrientation`, and `PolylinePacker::AddPoint` retains ribbon-orientation code, but Jamboard's active `PectinContainer` bridge never calls `AXIS_TILT` or `getOrientation`.)*

### 2. Open-Ended Universal Eraser Size Bands (`DrawingController::TouchTypeFromSize` @ `0x00474714`)
On Jamboard hardware (`pectin::device::Device::type == 3`, initialized unconditionally at `0x00280e24`), `DrawingController::TouchTypeFromSize(float size)` classifies `MotionEvent.getSize(i)` against two normalized float thresholds **with no upper cap**:
- **`size < 0.40f` (`0x3ecccccd`) → `2` (Stylus / Draw):** Fine-tip or tilted chisel strokes route to `UpdateDataFeedsWithTouchPoint` (`0x00475048`).
- **`0.40f <= size < 0.475f` (`0x3ef33333`) → `1` (Finger):** Standard touch/UI manipulation—or finger-wipe erasing when `CanUseFingerEraser()` (`KioskSettingsManager::FingerEraser`) is enabled.
- **`size >= 0.475f` (open-ended `[0.475f, 1.0+]`, no upper bound) → `3` (Universal Eraser):** Routes to `UpdateEraserTouchGroups` (`0x00474754`). Because there is no upper size limit, Google's official microfiber puck, **any third-party felt whiteboard eraser**, a folded cloth, or **the heel/side of a user's palm** all trigger hardware-class erasing identically.

### 3. Multi-Contact Palm & Whiteboard-Block Clustering (`UpdateEraserTouchGroups` @ `0x00474754`)
Because a palm wipe or a worn rectangular whiteboard eraser often splits into multiple adjacent IR shadow blobs (e.g., palm heel + knuckles, or two edges of a felt block):
- **Spatial Capture Radius (`200.0f * dpi_scale` px @ `0x004749e8`, ~63 mm on the 55" 4K panel):** Any contact landing within `200 px` of an active eraser `TouchGroup::GetCenterPoint()` (`0x00476da8`) is merged into that same `TouchGroup` (`TouchGroup::AddPoint` @ `0x00476d50`) rather than spawning stray finger gestures or pen strokes.
- **Dynamic Swath Width (`TouchGroup::GetRadius` @ `0x00476e04` & `EraserRenderer::CalculateStrokeThickness` @ `0x004a5dd0`):** `GetRadius()` computes the maximum Euclidean spread (`max_i hypot(pt_i - center)`, capped at `450.0f` px @ `0x00475e20`) across all contacts in the group and scales the eraser brush dynamically:
  $$\text{thickness} = 28\text{ px} + 2.0 \times \text{group\_radius} + 0.001 \times \text{speed}^2$$

### 4. Passive Stylus Wedge ("Tilt") Width via Optical `Tip Pressure` + Shadow Geometry
Because an optical InGlass bezel measures infrared beam occlusion rather than mechanical downward force:
- Rolling the passive stylus onto its flat ~15–18 mm chisel/wedge shoulder increases RAPT's reported **`Tip Pressure` (`0x0D:0x30`, `0..29999`)** as well as **`Width` / `Height` (`0x0D:0x48` / `0x49`)**.
- Stock `JamKiosk` feeds `MotionEvent.getPressure(i)` (`TouchPoint.pressure_`) into `LocalDataFeed::AppendPoint` (`0x0052b3fc`) → `ParticleRenderer::FeedPoint` (`0x004b57e8`), interpolating stroke width between `min_width` and `max_width`.
- For standard desktop Linux (GNOME Wayland, Rnote, Xournal++, Krita) and Android canvas apps, `jamboard-pen-daemon` **both** passes RAPT's raw `Tip Pressure` (`0..29999` → `ABS_PRESSURE` `256..4095`) **and** synthesizes `ABS_TILT_X` / `ABS_TILT_Y` (`0°..70°`) from `major_mm = max(w_mm, h_mm)` (`4.7 mm .. 18.5 mm`), while mapping palm/eraser cluster span (`32 mm .. 180 mm`) into `ABS_PRESSURE` when `BTN_TOOL_RUBBER` is active.

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
