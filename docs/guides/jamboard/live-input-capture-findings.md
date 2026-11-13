---
type: Guide
status: Active
system: Google Jamboard (baracus)
category: Guides
title: Live Input Capture Findings for the Google Jamboard RAPT Touch Controller (LineageOS)
tags: [jamboard, baracus, rapt, hid, hidraw, stylus, eraser, palm, lineageos, empirical, calibration]
date: 2026-11-13
related_to:
  - ./rapt-touch-and-scaler-protocol.md
  - ./stock-rom-reverse-engineering-findings.md
  - ./fedora-gnome-and-stylus-bringup.md
  - ../../../artifacts/jamboard/README.md
references:
  - https://github.com/LineageOS/android_device_google_baracus
---

# Live Input Capture Findings for the Google Jamboard RAPT Touch Controller (LineageOS)

## Overview

These are measurements taken from a live Jamboard running `lineage-22.2-20261109-NIGHTLY-baracus_tab` (Android 15, kernel `4.9.141`, `userdebug`, SELinux Enforcing), captured over Root ADB from `/dev/hidraw*`. They replace the size-band assumptions derived from static analysis of the stock ROM and are the basis for the classification in `artifacts/jamboard/jamboard-pen-daemon.c`.

Captures were taken with a finger, the stock passive stylus (upright and tilted), a **substitute whiteboard eraser** (the official Jamboard eraser was not available), the heel of a palm, and the stylus with a palm resting beside it. About 34,100 contact rows were analyzed. Small derived artifacts are in [`artifacts/jamboard/captures/`](../../../artifacts/jamboard/captures/): both HID report descriptors (raw and decoded) and `phase-summary.csv`. The multi-megabyte raw captures were not committed.

## Prerequisites

- LineageOS `baracus_tab` with **Settings > Developer options > Rooted debugging** (ADB only) enabled, then `adb root`.
- An unprivileged `adb shell` (uid 2000) can run `getevent` but is **blocked by SELinux** from `/dev/hidraw*`, `/proc/bus/input/devices`, `dmesg` and `/sys/kernel/debug/hid`. Root is required for any capture.
- `artifacts/jamboard/rapt-live-probe.sh` to reproduce the data.

## Findings

### 1. One USB device, two HID interfaces with different descriptors

`2986:0137` ("RAPT Touch RAPT Touch", driver `hid-multitouch`) exposes `usb-70090000.xusb-4/input0` (`hidraw0`) and `/input1` (`hidraw1`). Both `evdev` nodes show `INPUT_PROP_DIRECT`, 24 slots, `TOUCH_MAJOR/MINOR` `0..14999`, `POSITION_X/Y` `0..29999`, `PRESSURE` `0..29999`, and **no `ABS_MT_TOOL_TYPE`, `BTN_TOOL_PEN` or `BTN_TOOL_RUBBER`**. Android therefore sees every contact as a finger.

| | `hidraw0` (basic) | `hidraw1` (extended) |
| :--- | :--- | :--- |
| Descriptor | 2,313 B | **3,149 B** |
| Report `0xF1` | 340 B (24 x 14 B) | **484 B (24 x 20 B)** |
| Extra input report | none | `0xF6`, 103 B, contamination map (~33 Hz, streams even when idle) |
| Tool class field | no | **`Vendor 0xFF00:0x02` in `byte0` bits 1..7** |

The basic descriptor matches the stock ROM's `rapt_hid_descriptor.bin` except for the physical size. The extended descriptor was not present in the stock ROM dump. Full layouts are in [rapt-touch-and-scaler-protocol.md](./rapt-touch-and-scaler-protocol.md).

### 2. The firmware classifies every contact

On the extended interface `Vendor 0xFF00:0x02` takes the values `1` = stylus, `2` = finger, `3` = eraser/large. It is latched per contact ID and never changed mid-stroke in any capture.

### 3. `Width + Height` is exactly constant per class

| Firmware class | `Width + Height` (all rows) |
| :--- | :--- |
| `1` stylus | `12000` |
| `2` finger | `26998` |
| `3` eraser | `29998` |

`Height` (which equals `Vendor 0xFF00:0x01`) is the real size measure and `Width` is `class_base - Height`. A classifier of `Width + Height < 18000` -> stylus, `< 27000` -> finger, otherwise eraser reproduced the firmware class with **zero mismatches** on the stylus, eraser-substitute and pen+palm captures, and also works on the basic interface, which lacks the class field.

### 4. Per-phase results (from `phase-summary.csv`)

| Phase | Firmware class | Rows | Max simultaneous contacts | `Height` p50 / max | `Tip Pressure` p50 / max |
| :--- | :--- | ---: | ---: | :--- | :--- |
| Finger | 2 | 5,002 | 1 | 1866 / 1960 | 3617 / 8153 |
| Stylus (upright) | 1 | 4,716 | 1 | 1388 / 1615 | 5063 / 7799 |
| Stylus (tilted) | 1 | 4,458 | 1 | 1411 / 1647 | 5795 / 12491 |
| Stylus (tilted) | 2 | 203 | 1 | 1843 / 1952 | 1916 / 3089 |
| Substitute eraser | 3 | 3,295 | 3 | 1380 / 1803 | 2003 / 4271 |
| Substitute eraser | 2 | 3,527 | 5 | 1286 / 1819 | 701 / 1697 |
| Palm | 3 | 2,459 | 2 | 1694 / 1952 | 18983 / 29999 |
| Palm | 2 | 2,098 | 3 | 1639 / 1960 | 8372 / 29999 |
| Stylus + resting palm | 1 | 2,432 | 4 | 1333 / 1764 | 12257 / 29999 |
| Stylus + resting palm | 2 | 5,912 | 4 | 1552 / 1945 | 11924 / 29999 |

Takeaways:

- **Erasers and palms split into several blobs.** Eraser and palm frames carry up to 5 simultaneous contacts, and the secondary blobs are often classified as *finger* next to a class-3 blob. Without clustering they would show up as stray touches. This is why the daemon merges nearby contacts into one eraser group (`--eraser-cluster-mm 70`).
- **A resting hand beside the stylus was never class 3.** In the stylus + palm phase the hand registered only as class 2 (finger), and the stylus stayed class 1.
- **A very tilted stylus can flip to finger class** (203 rows) when its contact becomes broad enough. This is a known limitation; wedge/tilt handling is out of scope for now.
- **`Tip Pressure` is contact area, not force.** It rises with palm/eraser contact and reaches `29999`.
- `Vendor 0xFF00:0x03` is constant `4096`, and `0xFF00:0x04/0x05` duplicate `X/Y` in all captures.

### 5. The stock `0.40` / `0.475` size bands do not match live data

Static analysis of `libjamkiosk.so` found `TouchTypeFromSize` thresholds of `0.40` and `0.475` on `MotionEvent.getSize()`. With the live values, `0.5 * (Width + Height) / 14999` is `0.40` (stylus), `0.90` (finger) and `1.00` (eraser): the stylus sits exactly on the stock threshold and finger and eraser both land above `0.475`. The stock Google kernel (`LNX` boot image, not dumped) must therefore remap these HID fields differently from LineageOS's `hid-multitouch`. The daemon uses the firmware class or the `Width + Height` classifier instead, and the former `--use-norm-bands` option was removed.

### 6. Other measured facts

- **Physical size is `1207.6 mm x 678.4 mm`** (descriptor `Physical Max` `12076` / `6784`), not `1210 x 680` as in the stock ROM descriptor.
- Both interfaces report `0xF1` at ~357 Hz while touched; scan time advances 25 units (2.5 ms) per frame. `hidraw0` is silent when idle.
- `hazeldos-ctrl` is stopped after boot (`init.svc.hazeldos-ctrl=stopped`); touch is already powered, and props show `ro.boot.touch_panel_id=63`.
- `/vendor/etc/input-port-associations.xml` maps `usb-70090000.xusb-4/input0` to display 0 and `/input1` to display `99`.
- `/dev/ttyACM0` and `/dev/ttyACM1` exist, `/dev/ttyTHS1` is root only, and there is no `/dev/hidg0`.

## Common Pitfalls

- **Running the probe without root:** `cat /dev/hidraw*` silently fails under SELinux in an unprivileged `adb shell`. Use `adb root` first.
- **Assuming `hidraw0` and `hidraw1` have the same layout:** frame lengths differ (340 vs. 484 bytes) and `hidraw1` interleaves 103-byte `0xF6` frames. Parse by report ID and fixed length.
- **Treating `Tip Pressure` as force:** it follows contact area.
- **Using the substitute-eraser numbers as the official eraser's:** the official Jamboard eraser has not been measured. Class `3` should still apply, but this has not been confirmed.

## Open Items

- Capture the official Jamboard eraser (class and size).
- Stylus tilt/wedge behavior (deferred).
- Dump the stock `LNX` boot image to confirm how Google's kernel maps the vendor fields.
- Re-run the probe on Fedora (mainline kernel) once it boots, to confirm the same two interfaces and descriptors appear.

## References

- [`LineageOS/android_device_google_baracus`](https://github.com/LineageOS/android_device_google_baracus) - Device tree, `hazeldos-ctrl` init and `input-port-associations.xml`.
