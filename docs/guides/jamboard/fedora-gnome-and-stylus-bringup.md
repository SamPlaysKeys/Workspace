---
type: Guide
status: Active
system: Google Jamboard (baracus)
category: Guides
title: Fedora Workstation (GNOME) & Stylus/Eraser Bring-Up Plan for Google Jamboard
tags: [jamboard, baracus, fedora, gnome, aarch64, tegra210, u-boot, lineageos, emmc, stylus, eraser, install]
date: 2026-11-13
related_to:
  - ./stock-rom-reverse-engineering-findings.md
  - ./live-input-capture-findings.md
  - ./rapt-touch-and-scaler-protocol.md
references:
  - https://wiki.lineageos.org/devices/baracus/install/
  - https://github.com/LineageOS/android_device_google_baracus
  - https://github.com/LineageOS/android_hardware_nvidia_u-boot
  - https://github.com/LineageOS/android_hardware_nvidia_t21x_lineage
  - https://github.com/pb4ugoout/Jamboard
  - https://github.com/TheFitzZZ/jamboard-brightness-baracus
---

# Fedora Workstation (GNOME) & Stylus/Eraser Bring-Up Plan for Google Jamboard (`baracus`)

## Overview

This guide documents the end-to-end hardware architecture, reverse-engineering workflow, and installation procedure for running **stock Fedora Workstation (`aarch64`, GNOME on Wayland)**—with full multi-touch, passive stylus, physical eraser, and display backlight support—on a **Google Jamboard** (codename `baracus`).

Because the Jamboard's passive stylus and eraser currently lack complete tool-type/eraser handling in open-source ROMs, this plan uses a **two-stage execution strategy**:

1. **Stage 1 (LineageOS Lab & Protocol Capture):** Unlock the bootloader, back up Google's stock Android 7 partitions (preserving proprietary calibration, firmware, and the stock Jamboard APK's optical touch math), boot **LineageOS 22 (`baracus_tab`)**, and capture raw USB HID / `evdev` optical sensor data over Root ADB to build a universal `uinput` pen/eraser daemon.
2. **Stage 2 (Fedora Workstation `aarch64` Bring-Up on Internal eMMC):** Chainload a mainline U-Boot from Google's signed `cboot` bootloader, deploy stock Fedora Workstation (GNOME Wayland) **100% onto the internal 16 GB eMMC** (`CAC` for `/boot/efi`, `APP` for `/boot`, and `UDA` with `zstd` Btrfs compression for `/`), and enable the Jamboard hardware daemons (touchscreen GPIO init, UART scaler brightness, and the pen/eraser translator).

---

## Hardware Architecture & Reference Map

### Compute & Carrier Subsystems
- **SoC Module:** Standard 400-pin Samtec **NVIDIA Jetson TX1 (`Tegra210` / `T210`)** module on a custom Google/BenQ carrier board (`id=2180, sku=9999`).
  - **CPU / RAM / Storage:** 4x ARM Cortex-A57 @ 1.9 GHz (`aarch64`), 4 GB LPDDR4, 16 GB internal eMMC (`sdhci-tegra.3` / `sdhci@700b0600`).
  - **GPU:** 256-core NVIDIA Maxwell (`gm20b`), supported upstream by `tegra-drm` + `nouveau`.
- **55" 4K 60Hz Display & MStar MST9U23T1 Scaler:**
  - Video path: Tegra X1 `sor1` HDMI output → internal carrier HDMI → **MStar MST9U23T1 scaler** → 51-pin eDP to the 4K LCD panel.
  - External video inputs (2x HDMI + side USB-C DisplayPort alt-mode) also feed into the MStar scaler.
  - **Backlight & Input Switching:** There is **no PWM/GPIO/DDC backlight**. Brightness, video source switching, and speaker unmuting are controlled via 9600-baud UART commands sent over **`/dev/ttyTHS1`** (`serial@70006040`).
- **Optical Touchscreen ("HazelRed" / `hazeldos` — RAPT Touch InGlass):**
  - 100–120 Hz infrared optical waveguide bezel driven by an internal NXP/RAPT Touch controller (this unit reports `VID:PID 2986:0137`, driver `hid-multitouch`; Google's stock descriptor uses `1cbe:0003`, so handle both) wired to internal USB host port **`usb-70090000.xusb-4`** (`usb2-3` UTMI).
  - **Passive Stylus & Eraser:** Neither accessory contains electronics or batteries. The bezel measures the physical 2D optical shadow/ellipse on the glass (~1.5–3 mm fine stylus tip, ~8–15 mm finger, ~40–60 mm eraser puck).
  - **Power/Boot Sequence Required:** On every boot, the host must toggle three Tegra GPIOs (`pwren=174` / `V6`, `bootmode=184` / `X0`, `reset=185` / `X1`) to boot the touch MCU from SPIFI flash before it enumerates on USB.

### Physical Port & Button Identification (For Hands-On Operator)

| Port / Control | Location | Function | Important Notes |
| :--- | :--- | :--- | :--- |
| **Micro-USB Service Port** | **Rear / Back I/O bay** | **Fastboot & USB ADB** | The `cboot` fastboot port. In Linux the gadget controller is `700d0000.xudc` on padctl `usb2-0` (`mode = "otg"`); the EHCI nodes (`usb@7d000000`, `usb@7d004000`) are absent/disabled in both the vendor and mainline DTs. **Use this port for flashing and wired ADB.** |
| **USB-C Port** | Side bezel | DisplayPort Video-In ONLY | **Cannot** be used for Fastboot or ADB. |
| **2x USB 3.0 Type-A** | Side bezel | Host USB (Keyboard, Mouse, SSD) | Driven by `tegra-xusb` (`xusb@70090000`). Active once Linux loads `nvidia/tegra210/xusb.bin`. |
| **RJ-45 Gigabit Ethernet** | Rear I/O bay | **USB LAN** `0955:09ff` behind `xhci-tegra` | This board has **no PCIe bus** (`/sys/bus/pci/devices` is empty), so `r8169` never binds. The NIC is a USB device driven by `r8152`; it needs `phy-tegra-xusb`, `xhci-tegra`, `usbnet` and `r8152`, plus `nvidia/tegra210/xusb.bin`. |
| **Power / Vol+ / Vol-** | Side/Rear edge | Power & Bootloader navigation | Hold **Power for 4 seconds** from powered-off state to enter `cboot` Fastboot menu. |

### Internal 16 GB eMMC Partition Map (`sdhci-tegra.3`)

| Partition Name | Android Mount | Size | Block node (`mmcblk0`) | Role in LineageOS (Stage 1) | Role in Fedora GNOME (Stage 2) |
| :--- | :--- | :--- | :--- | :--- | :--- |
| `LNX` (`boot`) | `/boot` | 25.5 MiB | `p18` | U-Boot payload booting Android | `cboot`-compatible U-Boot (`u-boot-baracus.img`) |
| `SOS` (`recovery`) | `/recovery` | 25.5 MiB | `p16` | LineageOS Recovery (Root ADB) | **Keep LineageOS Recovery** as permanent rescue tool |
| `DTB` (`dtb`) | *(raw)* | ~4 MiB | `p17` | Bootloader/CBoot DTB | Keep LineageOS `dtb.img` |
| `CAC` (`cache`) | `/cache` | **256 MiB** | `p20` | Android cache | **Fedora `/boot/efi`** (`vfat` ESP) |
| `APP` (`system`) | `/system` | **2.0 GiB** | `p19` | LineageOS `/system` | **Fedora `/boot`** (`ext4`: `vmlinuz`, `initramfs`, `dtb`) |
| `vendor` | `/vendor` | ~540 MiB | — | Proprietary blobs | **Do not wipe** (preserve reference blobs) |
| `RP3` | `/mnt/vendor/wifi_config` | Small `ext4` | — | Per-unit Wi-Fi MAC & RF trim | **NEVER WIPE** (unit-unique calibration) |
| `UDA` (`userdata`) | `/data` | **11.33 GiB** (`12,163,120,128` B) | `p29` | Android `/data` | **Fedora `/` + `/home`** (`btrfs`, `compress=zstd:3`) |

> **Two naming traps:**
> 1. `cboot` fastboot only knows the **Android** names `boot`, `recovery`, `dtb`, `cache`, `system`, `userdata`. Uppercase GPT names (`CAC`, `APP`, `UDA`, `LNX`) fail with *"No partition present with this name"*.
> 2. U-Boot parses partition numbers as **hex** (`hextoul()` in `disk/part.c`). The ESP is `mmc 0:14` (`p20`) and `/boot` is `mmc 0:13` (`p19`); a loop over `1 2 ... 12` never reaches them.
>
> MMC numbering is also kernel-dependent: Recovery sees the eMMC as `mmcblk0`, Fedora as `mmcblk1`. Locate storage by UUID or partlabel, never by a hard-coded `mmcblk0`.

---

## Prerequisites

### Hands-On Operator Workstation & Gear
- Linux or macOS laptop with current `android-platform-tools` (`adb`, `fastboot`, `img2simg`, `mkbootimg`) plus a Linux environment (native or VM) with `btrfs-progs`, `dosfstools`, `rsync`, `zstd`, `python3`, and `losetup` for Stage 2 image assembly.
- Known-good **Micro-USB data cable** (verify it passes data, not power-only) connected to the Jamboard's **rear Micro-USB port**.
- Wired **USB keyboard** (and mouse) plugged into a **side USB 3.0 Type-A port**.
- At least **35 GB of free disk space** on the build computer (to hold the ~15 GB stock partition backups plus the unpacked Fedora raw image and sparse `UDA` image).
- **Jamboard Passive Stylus** and **Jamboard Physical Eraser** on hand for Stage 1 testing.

### Required Downloads for Stage 1 (LineageOS)
1. **LineageOS `baracus_tab` (Tablet variant — strongly preferred over Android TV `baracus`):**
   - Download `dtb.img`, `recovery.img`, and the latest `lineage-22.2-*-baracus_tab-signed.zip` from the LineageOS `baracus` / `baracus_tab` downloads page.
   - *(Why `baracus_tab` instead of `baracus`?)* `baracus` is an Android TV build that requires pairing a Bluetooth remote to navigate setup; `baracus_tab` is standard tablet Android and works immediately with touch/mouse/keyboard.
2. *(Only if the Jamboard is NOT already on Google's final EoL Android 7.0 release `NRT1.240624.004/12020658`):*
   - Google's final OTA `a40bffa0422296aa577c32d91fdf9f546301ce9c.zip` + stock recovery/DTB (see LineageOS `baracus` install wiki).

---

## Stage 1: Unlock, Backup Stock ROM, & Reverse-Engineer Stylus/Eraser on LineageOS

### Step 1.1: Verify Stock Firmware Build
1. Boot the Jamboard into stock Android 7.0.
2. If stuck on the unlicensed kiosk splash screen, plug a USB keyboard into the side USB-A port and press **`Super + N`** (`Windows + N`) or open notifications/Settings.
3. Navigate to **Settings → About** and confirm the build number ends in **`NRT1.240624.004`** (`12020658`).
   - *If on an older build, follow the LineageOS Wiki firmware update instructions before proceeding.*

### Step 1.2: Enter Fastboot & Unlock the Bootloader
1. Power off the Jamboard completely.
2. Plug the Micro-USB cable from your computer into the **rear Micro-USB service port**.
3. Hold the **Power** button for **~4 seconds** until the `cboot` Fastboot menu appears on screen.
4. On your computer, confirm connectivity:
   ```bash
   fastboot devices
   ```
5. Unlock the bootloader:
   ```bash
   fastboot oem unlock
   ```
6. Use the physical **Volume** buttons on the Jamboard to highlight confirmation and press **Power** to confirm.
   - **Important:** `fastboot oem unlock` **only** wipes `UDA` (`userdata`) and `CAC` (`cache`). Google's stock `APP` (`/system`), `vendor`, and `RP3` partitions remain completely intact!

### Step 1.3: Flash LineageOS Recovery & Back Up Stock Google Partitions (CRITICAL — DO NOT SKIP!)
Before installing LineageOS (which overwrites `APP` `/system`), boot into LineageOS Recovery to pull a full bit-for-bit backup of Google's stock system, vendor, and Wi-Fi calibration partitions.

1. Re-enter Fastboot mode (hold **Power** for 4 seconds from off) and flash LineageOS's `dtb.img` and `recovery.img`:
   ```bash
   fastboot flash dtb dtb.img
   fastboot flash recovery recovery.img
   ```
2. On the Jamboard's `cboot` screen, use the **Volume** buttons to highlight **"Boot recovery kernel"** and press **Power**.
3. Wait 5–10 seconds for LineageOS Recovery to initialize the display and touchscreen.
4. In LineageOS Recovery, go to **Advanced → Enable ADB**.
5. On your computer, create a backup folder and pull the raw stock partitions:
   ```bash
   mkdir -p ~/jamboard-stock-backup && cd ~/jamboard-stock-backup

   adb root || true
   adb shell ls -l /dev/block/platform/sdhci-tegra.3/by-name/

   # Pull Wi-Fi MAC/calibration, vendor blobs, boot/dtb, and stock Google /system
   adb pull /dev/block/platform/sdhci-tegra.3/by-name/RP3    stock-RP3-wifi.img
   adb pull /dev/block/platform/sdhci-tegra.3/by-name/DTB    stock-DTB.img
   adb pull /dev/block/platform/sdhci-tegra.3/by-name/LNX    stock-LNX-boot.img
   adb pull /dev/block/platform/sdhci-tegra.3/by-name/vendor stock-vendor.img
   adb pull /dev/block/platform/sdhci-tegra.3/by-name/APP    stock-APP-system.img

   sha256sum *.img > SHA256SUMS.txt
   ```
   > **Why `stock-APP-system.img` is essential:** Mounting this `ext4` image gives us `/system/usr/idc/*.idc` (Google's exact touch calibration), `/system/bin/scalerdaemon` (all 66 UART scaler/audio/input commands), `libinputflinger.so`, and the stock **Google Jamboard APK**—which contains Google's exact mathematical formulas and size thresholds for stylus, stroke width, palm rejection, and physical eraser detection!

### Step 1.4: Install LineageOS 22 (`baracus_tab`) & Enable Root + Wireless ADB
1. Still in LineageOS Recovery on the Jamboard:
   - Tap **Factory reset → Format data/factory reset** and confirm.
   - Return to the main menu and tap **Apply update → Apply from ADB**.
2. Sideload the LineageOS `baracus_tab` package from your computer:
   ```bash
   adb sideload lineage-22.2-*-baracus_tab-signed.zip
   ```
3. Tap **Reboot system now**.
4. Once booted into LineageOS:
   - Connect to Wi-Fi.
   - Go to **Settings → About tablet** and tap **Build number** 7 times to unlock Developer Options.
   - Go to **Settings → System → Developer options** and enable:
     - **USB debugging**
     - **Rooted debugging** *(critical — lets `adb root` inspect `/dev/hidraw*`, `/dev/input/*`, and `/sys/kernel/debug/hid`)*
     - **Wireless debugging** / **ADB over network** *(optional, lets you disconnect the rear Micro-USB cable and connect via `adb connect <JAMBOARD_IP>:5555`)*.

---

### Step 1.5: Reverse-Engineering the Stylus & Eraser Protocol

#### Background Findings Driving the Investigation
- The RAPT Touch USB device (`2986:0137` on this unit, `1cbe:0003` for Google's stock variant, at `usb-70090000.xusb-4`) exports large USB HID Report Descriptors (2,313 B and 3,149 B) and **multiple USB HID interfaces** (`input0` and `input1`).
- Currently, LineageOS silences `input1` by mapping it to non-existent `display="99"` in `input-port-associations.xml` because both interfaces fire concurrently.
- Furthermore, standard `hid-multitouch` reports contacts as generic touches with `ABS_MT_TOUCH_MAJOR/MINOR` and no tool type, which neither AOSP nor GNOME automatically promotes into a hardware `Stylus` or `Eraser`. The live unit also exposes a second, extended 3,149-byte HID interface carrying the firmware tool class.

#### A. Static Extraction from the Stock Backup (Run on Computer)
Mount `stock-APP-system.img` read-only to extract Google's reference configs and binaries:
```bash
mkdir -p /tmp/mnt-system ~/jamboard-re/stock-files
sudo mount -o ro,loop ~/jamboard-stock-backup/stock-APP-system.img /tmp/mnt-system

cp -a /tmp/mnt-system/usr/idc ~/jamboard-re/stock-files/ 2>/dev/null || true
cp -a /tmp/mnt-system/usr/keylayout ~/jamboard-re/stock-files/ 2>/dev/null || true
cp -a /tmp/mnt-system/bin/scalerdaemon ~/jamboard-re/stock-files/ 2>/dev/null || true
find /tmp/mnt-system -iname "*jam*" -o -iname "*hazel*" -o -iname "*rapt*" -o -iname "*mikado*" \
  > ~/jamboard-re/stock-files/interesting-paths.txt

sudo umount /tmp/mnt-system
```

#### B. Baseline HID & Input Topology Dump (Run via Root ADB on LineageOS)
```bash
mkdir -p ~/jamboard-re/live-dumps && cd ~/jamboard-re/live-dumps
adb root && adb wait-for-device

# 1. Input subsystem devices & Android InputReader state
adb shell cat /proc/bus/input/devices > input-devices.txt
adb shell dumpsys input > dumpsys-input.txt
adb shell ls -la /dev/input/ /dev/hidraw* > dev-nodes.txt

# 2. Full 2,313-byte HID Report Descriptor(s) and live HID debug state
adb shell 'for f in /sys/kernel/debug/hid/*/rdesc; do echo "=== $f ==="; cat "$f"; done' > hid-rdesc.txt
```
*(Inspect `hid-rdesc.txt` with `hid-recorder` / `hidrd-convert` to enumerate every Digitizer, Pen, Eraser, Width/Height, Azimuth, and Vendor `0xFF00` usage across `input0`, `input1`, and `/dev/hidraw*`.)*

#### C. Guided Live Capture (`rapt-live-probe.sh`)
Enable **Developer options > Rooted debugging**, run `adb root`, then use the helper script from this repo. It finds the extended RAPT `hidraw` interface, records five phases (finger, stylus, eraser/whiteboard block, palm, stylus + resting palm) as CSVs, and prints per-class statistics:

```bash
PROBE_SECS=12 ./artifacts/jamboard/rapt-live-probe.sh ./jamboard-probe-results
```

The measured results from this procedure are in [live-input-capture-findings.md](./live-input-capture-findings.md). The key result: the RAPT firmware classifies contacts itself (`1` stylus, `2` finger, `3` eraser/large) and `Width + Height` is constant per class, so no size-guessing geometry is required.

#### D. Universal `jamboard-pen-daemon` (`hidraw` -> `uinput`)
`artifacts/jamboard/jamboard-pen-daemon.c` is a small static C daemon that works identically on **both LineageOS and Fedora GNOME**:
- Reads RAPT `hidraw` frames directly (extended interface preferred) and grabs the raw `evdev` nodes with `EVIOCGRAB` so duplicate events never leak to the OS.
- Exposes two clean Linux `uinput` devices (`INPUT_PROP_DIRECT`):
  1. **Jamboard RAPT Finger Touch:** finger-class contacts as 24-slot multi-touch.
  2. **Jamboard RAPT Stylus & Eraser:** stylus-class contacts -> `BTN_TOOL_PEN` + `BTN_TOUCH` + `ABS_X/Y/PRESSURE`; eraser-class contacts (any eraser, felt block, or palm) -> **`BTN_TOOL_RUBBER`** with spatial clustering so palm wipes do not leak finger gestures.
- **Result:** Android (`TOOL_TYPE_STYLUS` / `TOOL_TYPE_ERASER`) and Fedora GNOME Wayland (`libinput` tablet tools) switch between Pen, Finger, and Eraser with no per-app hacks. Stylus tilt/wedge handling is not implemented (the optical bezel reports no tilt).

---

## Stage 2: Installing Stock Fedora Workstation (`aarch64`, GNOME) on Internal 16 GB eMMC

### Internal eMMC Storage Architecture (`CAC` + `APP` + `UDA`)
To keep the Jamboard 100% self-contained with nothing plugged into its external USB ports, Fedora's three standard partitions map directly onto three existing Android GPT partitions on the internal 16 GB eMMC (`sdhci-tegra.3`)—without repartitioning the drive or touching Google's signed bootchain/calibration partitions:

| Fedora Mount | Filesystem | Target Android Partition | Partition Size | Notes |
| :--- | :--- | :--- | :--- | :--- |
| *(U-Boot chainload)* | Android `boot.img` (v0) | `LNX` (`boot`) | 25.5 MiB | Chainloads mainline U-Boot from stock `cboot` |
| *(Rescue OS)* | Android `boot.img` | `SOS` (`recovery`) | 25.5 MiB | **Keeps LineageOS Recovery** for permanent Root ADB rescue |
| `/boot/efi` | `vfat` (FAT32) | `CAC` (`cache`) | **256 MiB** | Fedora's EFI binaries + DTB use ~25 MiB |
| `/boot` | `ext4` | `APP` (`system`) | **2.0 GiB** | Exact 1:1 size match with Fedora's raw disk `/boot` partition |
| `/` & `/home` | `btrfs` (`zstd:3`) | `UDA` (`userdata`) | **11.33 GiB** (`12,163,120,128` B); image is 9400M | Fresh Fedora Workstation compresses from ~4.8 GiB down to **~2.8 GiB**; `btrfs filesystem resize max /` claims the remaining ~2.3 GiB |

---

### Step 2.1: Build a `cboot`-Compatible U-Boot (`u-boot-baracus.img`)
Google's Jetson TX1 SoC has PKC security fuses blown, preventing raw RCM/APX bootloader replacement, **but** unlocked stock `cboot` happily boots any unsigned Android `boot.img` from `LNX`. LineageOS already proved mainline-derived U-Boot chainloading from `cboot` (`LineageOS/android_hardware_nvidia_u-boot`).

Two measured constraints decide the image format:

| Constraint | Evidence |
| :--- | :--- |
| `cboot` only boots **boot image header v0** (shipped here with a dummy ramdisk; v0 without a ramdisk was not tested) | The stock LineageOS `boot.img` is header v0, page size 2048, kernel 759,592 B @ `0x10008000`, ramdisk 13,998,080 B @ `0x11000000`. A v1 header with **no** ramdisk is silently ignored: the panel stays on the Jamboard logo forever and nothing prints. |
| `CONFIG_BOOTSTD_FULL=y` is required for `bootflow scan -lb` | Without it U-Boot prints `Flags not supported: enable CONFIG_BOOTSTD_FULL` on its invisible console and stops at the prompt, which looks like a hang. |

On an `x86_64` or `aarch64` Linux build machine:
```bash
git clone https://github.com/LineageOS/android_hardware_nvidia_u-boot.git -b lineage-22.2
cd android_hardware_nvidia_u-boot

make ARCH=arm CROSS_COMPILE=aarch64-linux-gnu- baracus_defconfig
scripts/config --enable  BOOTSTD_FULL
scripts/config --disable TOOLS_MKEFICAPSULE   # needs gnutls headers; not present on an immutable host
```

Keep the FAT and bootstd options enabled, and set `CONFIG_BOOTCOMMAND`. The **verified** command loads the kernel from the ESP and calls `booti`; `bootflow scan -lb` is documented only as the UEFI-style alternative, and it needs U-Boot to read the ext4 `/boot` (unverified on this unit):
```ini
CONFIG_EFI_LOADER=y
CONFIG_CMD_BOOTEFI=y
CONFIG_BOOTSTD=y
CONFIG_BOOTSTD_FULL=y
CONFIG_BOOTSTD_DEFAULTS=y
CONFIG_BOOTMETH_EFI=y
CONFIG_FS_FAT=y
CONFIG_FS_EXT4=y
CONFIG_EFI_PARTITION=y
CONFIG_OF_BOARD_SETUP=y
CONFIG_OF_SYSTEM_SETUP=y
CONFIG_BOOTCOMMAND="setenv fdtfile nvidia/tegra210-baracus.dtb; if fatload mmc 0:14 ${kernel_addr_r} /KERNEL.IMG; then fatload mmc 0:14 ${fdt_addr_r} /BARACUS.DTB; fatload mmc 0:14 ${ramdisk_addr_r} /INITRD.IMG; setenv rdsz ${filesize}; booti ${kernel_addr_r} ${ramdisk_addr_r}:${rdsz} ${fdt_addr_r}; fi; reset"
```
```bash
make ARCH=arm CROSS_COMPILE=aarch64-linux-gnu- olddefconfig
```
`mmc 0:14` is the ESP: U-Boot reads the partition number as **hex**, and `CAC` is partition 20 (`0x14`). `bootcmd` ends in `reset`, so a failed load reboots instead of parking at the invisible prompt; without that, a failure is indistinguishable from a frozen panel.

An earlier `bootflow scan -lb` variant never found the extlinux flow, but that measurement is **not** evidence that U-Boot cannot read ext4: the loop used then (`for i in 1 2 ... 12`) is parsed as hex and only probed partitions 1-9 and 16-18, so `APP` (`p19`) was never scanned. U-Boot's own `include/ext4fs.h` lists `extents`, `64bit`, `flex_bg` and `filetype` as supported and treats `metadata_csum`/`csum_seed` as lazy read-only; `orphan_file` is a COMPAT feature and is ignored. To test the ext4/GRUB path explicitly:
```text
ext4load mmc 0:13 ${loadaddr} /extlinux/extlinux.conf   # APP is p19 = 0x13
```

Compile, then wrap `u-boot-dtb.bin` as a **v0** boot image with a dummy ramdisk:
```bash
make -j"$(nproc)" ARCH=arm CROSS_COMPILE=aarch64-linux-gnu-
head -c 4096 /dev/zero > dummy-ramdisk.bin
mkbootimg --kernel u-boot-dtb.bin --ramdisk dummy-ramdisk.bin \
  --header_version 0 --pagesize 2048 --base 0x10000000 \
  --kernel_offset 0x00008000 --ramdisk_offset 0x01000000 --tags_offset 0x00000100 \
  -o u-boot-baracus.img
```
Verified result: `ANDROID!`, header v0, page size 2048, kernel 761,280 B, `kernel_addr=0x10008000`, `tags_addr=0x10000100`.

> **The UEFI/GRUB path is unverified on this unit; the FAT ESP path is the verified one.** U-Boot loads the kernel, DTB and initramfs from `CAC` and calls `booti` (see Step 2.4). Two observations motivated the FAT path, and neither is conclusive: `bootflow scan -lb` never produced a flow (that scan was the hex/partition artefact described above), and a fallback `reboot` added to `EFI/BOOT/grub.cfg` never fired. The second test was also made with the **EFI zboot** `vmlinuz` still in the BLS entry, which GRUB cannot start, so GRUB may simply have failed on the kernel. Re-test both before writing either off.
>
> Also note that Fedora's `/boot/vmlinuz` is an **EFI zboot image** (`MZ` + `zimg`, zstd payload), which `booti` cannot start. Extract the plain `Image` first:
> ```bash
> dd if=vmlinuz-7.2.11-300.fc45.aarch64 of=p.zst bs=1 skip=100080 count=17226275
> zstd -d -o Image p.zst            # 78,905,344 B, magic "ARMd" at 0x38
> cp Image KERNEL.IMG               # ship UNCOMPRESSED (see note below)
> ```
>
> Do **not** gzip it unless you also set `kernel_comp_addr_r` and `kernel_comp_size`: `booti` refuses a compressed image without them (`cmd/booti.c`: *"kernel_comp_addr_r or kernel_comp_size is not provided!"*). The verified image was the uncompressed `Image` (78,905,344 B) named `KERNEL.IMG`.
> ```bash
> # only if you prefer a smaller file (unverified on this unit):
> #   gzip -9 -k Image ; setenv kernel_comp_addr_r 0xA0000000 ; setenv kernel_comp_size 0x1400000
> ```

---

### Step 2.2: Compile Mainline Device Tree (`tegra210-baracus.dtb`)
Stock Fedora `aarch64` `kernel-core` already enables `ARCH_TEGRA_210_SOC`, `tegra-drm`, `nouveau`, `xhci-tegra`, `sdhci-tegra`, `r8152`, `usbnet`, `brcmfmac`, and `serial-tegra`, and ships Jetson TX1's `tegra210-p2371-2180.dtb`. Build `tegra210-baracus.dtb` (adapting `LineageOS/android_hardware_nvidia_t21x_lineage`'s `tegra210-baracus.dts` onto mainline `tegra210-p2371-2180.dts`) with these five Jamboard overrides:
1. **HDMI output (`sor@54580000`, `nvidia,tegra210-sor1`):** `status = "okay"` with `avdd-io-hdmi-dp-supply`, `vdd-hdmi-dp-pll-supply`, `hdmi-supply`, `nvidia,ddc-i2c-bus` and `nvidia,hpd-gpio`; keep `dsi@54300000` disabled. There is **no** `hdmi@54280000` node in mainline TX1 device trees (0 occurrences in Fedora's `tegra210-p2371-2180.dtb`); the earlier `hdmi@54280000` / `avdd_hdmi-supply` naming was wrong.
2. **Internal Touch USB (`xusb@70090000` / `padctl@7009f000`):** Enable UTMI port `usb2-3` in host mode for the RAPT Touch controller.
3. **Scaler UART (`serial@70006040`):** Ensure UARTB (`/dev/ttyTHS1`) is `status = "okay"`.
4. **TrustZone / PMC Compatibility:** Omit `nvidia,secure-pmc` on `pmc@7000e400` if stock TZ monitor traps secure PMC writes.
5. **Memory:** the unit has **4 GiB** (`MemTotal: 4033116 kB` under Recovery), so the DTB memory node must say 4 GiB. U-Boot's `arch_fixup_fdt()` calls `fdt_fixup_memory_banks()` and may overwrite an edited size, so verify the value the kernel actually sees.

> Do not add `ip=...` boot arguments: `CONFIG_IP_PNP` is not set in Fedora's kernel, so they are ignored. Networking must come up through NetworkManager.

---

### Step 2.3: Unpack & Customize Stock Fedora Workstation `aarch64`
Download the official pre-installed ARM64 disk image (e.g., Fedora 44 or 45 Workstation):

```bash
wget https://download.fedoraproject.org/pub/fedora/linux/releases/45/Workstation/aarch64/images/Fedora-Workstation-Disk-45-1.4.aarch64.raw.xz
unxz Fedora-Workstation-Disk-45-1.4.aarch64.raw.xz
LOOP=$(sudo losetup -Pf --show Fedora-Workstation-Disk-45-1.4.aarch64.raw)
echo "Loop device: $LOOP"
# ${LOOP}p1 = ~600M FAT32 ESP (/boot/efi)
# ${LOOP}p2 = 2.0G ext4 (/boot)
# ${LOOP}p3 = Btrfs (subvolumes: root, home)
```

#### 1. Build the 256 MiB ESP Image (`fedora_esp.img` for `CAC`)
Preserve the exact FAT32 volume ID (`UUID`) from `${LOOP}p1` so `/etc/fstab` needs no UUID edits, then add the **FAT boot payload** that U-Boot loads (the ESP is the only filesystem U-Boot can read on this unit):
```bash
ESP_UUID_RAW=$(sudo blkid -s UUID -o value "${LOOP}p1" | tr -d '-')
truncate -s 256M fedora_esp.img
mkfs.vfat -F 32 -i "$ESP_UUID_RAW" fedora_esp.img

mkdir -p /tmp/esp-src /tmp/esp-dst
sudo mount -o ro "${LOOP}p1" /tmp/esp-src
sudo mount -o loop fedora_esp.img /tmp/esp-dst
sudo cp -a /tmp/esp-src/. /tmp/esp-dst/
sudo mkdir -p /tmp/esp-dst/dtb/nvidia
sudo cp tegra210-baracus.dtb /tmp/esp-dst/dtb/nvidia/

# FAT boot payload for the U-Boot booti path (8.3-compatible names)
sudo cp KERNEL.IMG  /tmp/esp-dst/KERNEL.IMG    # plain (uncompressed) arm64 Image
sudo cp INITRD.IMG  /tmp/esp-dst/INITRD.IMG    # Fedora initramfs
sudo cp tegra210-baracus.dtb /tmp/esp-dst/BARACUS.DTB
sudo umount /tmp/esp-src /tmp/esp-dst
```
`INITRD.IMG` must be the **firmware-trimmed** initramfs: the untrimmed Fedora initramfs is ~255 MB and cannot fit a 256 MiB ESP next to a ~79 MB kernel. Trimming only `usr/lib/firmware/*` (keeping `nvidia/tegra210`) brings it to ~73 MB and it reaches `initrd-root-fs.target` on this unit.

```bash
# trim, keeping only the Tegra210 firmware
zstd -dc initramfs-7.2.11-300.fc45.aarch64.img > base.cpio
python3 - <<'PY'
import sys
blob = open('base.cpio','rb').read(); off = 0; out = []
while off < len(blob) and blob[off:off+6] == b'070701':
    f = [int(blob[off+6+i*8:off+14+i*8], 16) for i in range(13)]
    namesize, filesize = f[11], f[6]
    name = blob[off+110:off+110+namesize-1].decode()
    total = 110 + namesize; total += (4 - total % 4) % 4
    end = off + total + filesize + ((4 - filesize % 4) % 4)
    entry = blob[off:end]; off = end
    if name == 'TRAILER!!!':
        continue
    if name.startswith('usr/lib/firmware/') and not name.startswith('usr/lib/firmware/nvidia/tegra210'):
        continue
    out.append(entry)
tr = bytes.fromhex('070701') + b'0'*104 + b'TRAILER!!!\0'
tr += b'\0' * ((4 - len(tr) % 4) % 4)      # newc entries are 4-byte aligned
open('base2.cpio','wb').write(b''.join(out) + tr)
PY
zstd -19 -T0 base2.cpio -o INITRD.IMG      # ~73 MB
```

ESP budget: kernel ~79 MB + `INITRD.IMG` ~73 MB + `BARACUS.DTB` ~0.1 MB = ~152 MB of the 256 MiB `CAC`, leaving ~100 MB. Keep it in mind before adding anything else to the ESP.

Verify the archive before flashing (this caught two real faults: a missing module set, and an `/init` that silently stayed a regular file):

```bash
lsinitrd INITRD.IMG | grep -E 'sdhci-tegra|cqhci|rpmb-core|^[-l].* init'
zstd -dc INITRD.IMG | cpio -i --to-stdout init | tail -c 40   # last /init entry wins
```

The kernel unpacker **refuses to replace a regular file with a symlink** (`vfs_symlink` returns `-EEXIST`), so appending `init -> usr/lib/systemd/systemd` over an existing regular `/init` silently keeps the old script. Build from a base with no `/init` override instead of appending one. Earlier "the initrd size causes a reset" conclusions were artefacts of the failed load and are not a reason to pad or split the initramfs.

#### 2. Customize `/boot` (`${LOOP}p2` → `fedora_boot.img` for `APP`)
```bash
mkdir -p /tmp/fed-boot
sudo mount "${LOOP}p2" /tmp/fed-boot
sudo mkdir -p /tmp/fed-boot/dtb/nvidia
sudo cp tegra210-baracus.dtb /tmp/fed-boot/dtb/nvidia/

# Append Tegra210/Nouveau kernel arguments to BLS entry
sudo sed -i '/^options / s/$/ cma=512M nouveau.atomic=1 clk_ignore_unused pd_ignore_unused console=tty0/' \
  /tmp/fed-boot/loader/entries/*.conf

sudo umount /tmp/fed-boot
sudo dd if="${LOOP}p2" of=fedora_boot.img bs=4M status=progress
```

#### 3. Build the Internal eMMC Rootfs Image (`fedora_uda_raw.img` for `UDA`)
The real `UDA` partition is **`12,163,120,128` bytes (`11.33 GiB`)** — about 2.3 GiB larger than the `9.4 GiB` quoted by older notes. A `9400M` (`9,856,614,400` bytes) Btrfs image is still the right choice: it fits with headroom and is grown on first boot. Create it with **the exact same UUID** as `${LOOP}p3`, mount it with **`compress=zstd:3`** so Fedora shrinks to ~2.8 GiB on disk, copy the `root` and `home` subvolumes into it, and inject the Jamboard hardware services:

```bash
ROOT_UUID=$(sudo blkid -s UUID -o value "${LOOP}p3")
truncate -s 9400M fedora_uda_raw.img
mkfs.btrfs -U "$ROOT_UUID" -L fedora fedora_uda_raw.img

mkdir -p /tmp/src-btrfs /tmp/dst-btrfs
sudo mount -o ro "${LOOP}p3" /tmp/src-btrfs
sudo mount -o loop,compress=zstd:3,noatime fedora_uda_raw.img /tmp/dst-btrfs

# Recreate Fedora's standard Btrfs subvolumes and copy all files + xattrs
sudo btrfs subvolume create /tmp/dst-btrfs/root
sudo btrfs subvolume create /tmp/dst-btrfs/home
sudo rsync -aAXH --info=progress2 /tmp/src-btrfs/root/ /tmp/dst-btrfs/root/
sudo rsync -aAXH --info=progress2 /tmp/src-btrfs/home/ /tmp/dst-btrfs/home/

sudo umount /tmp/src-btrfs
```

Now customize the target rootfs inside `/tmp/dst-btrfs/root` (let `FED_ROOT=/tmp/dst-btrfs/root`):

```bash
FED_ROOT=/tmp/dst-btrfs/root
```

- **A. Enable `zstd:3` Btrfs Compression & eMMC Space Hygiene:**
  ```bash
  # Add compress=zstd:3,noatime to Btrfs mounts in /etc/fstab
  sudo sed -i 's/subvol=root[^[:space:]]*/&,compress=zstd:3,noatime/' "$FED_ROOT/etc/fstab"
  sudo sed -i 's/subvol=home[^[:space:]]*/&,compress=zstd:3,noatime/' "$FED_ROOT/etc/fstab"

  # Cap systemd journal at 100M and keep max 2 kernels in /boot
  sudo mkdir -p "$FED_ROOT/etc/systemd/journald.conf.d"
  printf "[Journal]\nSystemMaxUse=100M\n" | sudo tee "$FED_ROOT/etc/systemd/journald.conf.d/size.conf" > /dev/null
  printf "installonly_limit=2\nkeepcache=False\n" | sudo tee -a "$FED_ROOT/etc/dnf/dnf.conf" > /dev/null
  ```

- **B. Early-Boot eMMC & Side USB 3.0 Firmware (`dracut`):**
  Create `$FED_ROOT/etc/dracut.conf.d/jamboard.conf`. The eMMC driver has a real dependency chain (`modinfo`), and loading only `sdhci-tegra` leaves the eMMC absent, so list the whole chain:
  ```conf
  add_drivers+=" i2c-tegra mmc_core sdhci sdhci-pltfm cqhci rpmb-core mmc_block sdhci-tegra phy-tegra-xusb xhci-tegra usbnet r8152 "
  # Fedora ships this firmware compressed; the kernel decompresses .xz
  # (CONFIG_FW_LOADER_COMPRESS_XZ=y), so include whichever form exists.
  install_optional_items+=" /usr/lib/firmware/nvidia/tegra210/xusb.bin.xz /usr/lib/firmware/nvidia/tegra210/xusb.bin "
  ```
  Dependencies: `sdhci-tegra` -> `mmc_core, sdhci, cqhci, sdhci-pltfm`; `sdhci` -> `mmc_core`; `mmc_block` -> `mmc_core, rpmb-core`; `r8152` -> `usbnet` -> `mii`. `btrfs` is built into the kernel (`CONFIG_BTRFS_FS=y`), so it needs no module.
  *(Inside an `aarch64` `systemd-nspawn` / `chroot` with `qemu-user-static`, run `dnf install -y libgpiod-utils && dracut --force --regenerate-all`—or run `dnf install -y libgpiod-utils` over Ethernet/USB keyboard right after first boot).*

- **C. Broadcom 802.11ac Wi-Fi NVRAM:**
  Copy the `bcm4354` NVRAM file extracted from `stock-vendor.img` / `stock-RP3-wifi.img` (or NVIDIA's `nvram_jetsonE_cv_4354.txt`) to:
  `$FED_ROOT/usr/lib/firmware/brcm/brcmfmac4354-sdio.txt`

- **D. Touchscreen GPIO Wake-Up Script, Unit & Scaler-TTY Tag:**
  Fedora 45 ships **libgpiod v2**, which removed the v1 `-c gpiochip0` syntax. `artifacts/jamboard/systemd/jamboard-touch-init` detects the version at run time and backgrounds `gpioset` itself, so `$!` names the process that owns the lines (a subshell or pipeline would leave the reset asserted forever). Install the artifacts as they are:
  ```bash
  sudo install -m 0755 artifacts/jamboard/systemd/jamboard-touch-init \
    "$FED_ROOT/usr/local/bin/jamboard-touch-init"
  sudo install -m 0644 artifacts/jamboard/systemd/jamboard-touch-init.service \
    "$FED_ROOT/etc/systemd/system/"
  sudo ln -sf /etc/systemd/system/jamboard-touch-init.service \
    "$FED_ROOT/etc/systemd/system/sysinit.target.wants/jamboard-touch-init.service"

  # Tag the scaler TTY: systemd only creates .device units for udev devices
  # that carry the "systemd" tag, and TTYs are not tagged by default.
  sudo install -m 0644 artifacts/jamboard/udev/99-jamboard-scaler-uart.rules \
    "$FED_ROOT/etc/udev/rules.d/"
  ```
  The script asserts reset (`185=1`), selects SPIFI boot (`184=0`), enables the active-low step-down power rail (`174=0`), waits 250 ms, then releases reset (`185=0`) and holds the rail on for the session.

- **E. MStar Scaler CLI & Boot Defaults (`/usr/local/bin/jamboard-scaler`):**
  Install `artifacts/jamboard/jamboard-scaler.sh` as `jamboard-scaler` together with its unit. It supersedes the older one-packet `jamboard-brightness` script and adds `input`, `mute`, `get-brightness` and `version`:
  ```bash
  sudo install -m 0755 artifacts/jamboard/jamboard-scaler.sh \
    "$FED_ROOT/usr/local/bin/jamboard-scaler"
  sudo install -m 0644 artifacts/jamboard/systemd/jamboard-backlight.service \
    "$FED_ROOT/etc/systemd/system/"
  sudo ln -sf /etc/systemd/system/jamboard-backlight.service \
    "$FED_ROOT/etc/systemd/system/multi-user.target.wants/jamboard-backlight.service"
  ```
  The unit runs `jamboard-scaler input internal` then `jamboard-scaler brightness 80`, and `Requires=dev-ttyTHS1.device`, so the udev rule from step D must be present or the unit never starts. *(Manual check after first boot: `jamboard-scaler get-brightness`.)*

- **F. Install `jamboard-pen-daemon` (from Stage 1) & Trigger SELinux Relabel:**
  Build the daemon as a static `aarch64` binary (it only needs `hidraw`, `evdev`, `uinput` and `poll`), install it and its unit from `artifacts/jamboard/`, and ensure Fedora's Enforcing SELinux policy labels the new files on first boot:
  ```bash
  sudo install -m 0755 jamboard-pen-daemon "$FED_ROOT/usr/local/bin/jamboard-pen-daemon"
  sudo install -m 0644 artifacts/jamboard/systemd/jamboard-pen-daemon.service \
    "$FED_ROOT/etc/systemd/system/"
  sudo ln -sf /etc/systemd/system/jamboard-pen-daemon.service \
    "$FED_ROOT/etc/systemd/system/multi-user.target.wants/jamboard-pen-daemon.service"
  sudo touch "$FED_ROOT/.autorelabel"
  ```
  The unit runs `/usr/bin/modprobe uinput` (a symlink to `kmod` on Fedora) before the daemon; the image kernel has `CONFIG_INPUT_UINPUT=m`.

- **G. Unmount & Build Byte-Exact RAW-Only Sparse Images:**
  `cboot`'s sparse writer **fails on `FILL` (`0xCAC2`) and `DONTCARE` (`0xCAC3`) chunks**: with an `img2simg` image the *last* part dies with `Partition write failed!`, and `DONTCARE` additionally leaves stale Android data in the skipped ranges (only safe for free space, never for file holes). Build sparse images that contain **only `RAW` chunks**, and verify them with a round trip:
  ```bash
  sudo umount /tmp/dst-btrfs
  sudo losetup -d "$LOOP"

  # system: one RAW chunk over all 512,000 x 4096 B blocks
  python3 - <<'PY'
  import os, struct
  src, dst, blk = "fedora_boot.img", "_boot_full.simg", 4096
  size = os.path.getsize(src); blocks = size // blk
  with open(dst, "wb") as o:
      o.write(struct.pack("<IHHHHIIII", 0xED26FF3A, 1, 0, 28, 12, blk, blocks, 1, 0))
      o.write(struct.pack("<HHII", 0xCAC1, 0, blocks, 12 + size))
      with open(src, "rb") as f:
          while (d := f.read(32 << 20)):
              o.write(d)
  PY

  # userdata: total_sz is 32-bit, so emit 512 MiB (131,072-block) RAW chunks
  python3 - <<'PY'
  import os, struct
  src, dst, blk = "fedora_uda_raw.img", "fedora_uda_full.simg", 4096
  size = os.path.getsize(src); blocks = size // blk
  step = 131072                      # 512 MiB per RAW chunk
  chunks = (blocks + step - 1) // step
  with open(dst, "wb") as o, open(src, "rb") as f:
      o.write(struct.pack("<IHHHHIIII", 0xED26FF3A, 1, 0, 28, 12, blk, blocks, chunks, 0))
      left = blocks
      while left:
          n = min(step, left)
          o.write(struct.pack("<HHII", 0xCAC1, 0, n, 12 + n * blk))
          o.write(f.read(n * blk))
          left -= n
  PY

  # prove the images are byte-exact before flashing
  simg2img _boot_full.simg /tmp/boot-check.img && cmp fedora_boot.img /tmp/boot-check.img
  simg2img fedora_uda_full.simg /tmp/uda-check.img   && cmp fedora_uda_raw.img /tmp/uda-check.img
  ```
  `fedora_esp.img` stays a plain 256 MiB FAT32 image; `fastboot` converts it itself. If that conversion ever misbehaves, build a RAW-only sparse copy of it with the first script and flash that instead.

---

### Step 2.4: Flash Everything to Internal eMMC via Fastboot

Enter `cboot` Fastboot mode (power off, connect the rear Micro-USB cable, hold **Power** for 4 seconds) and verify the device:
```bash
fastboot devices
fastboot getvar max-download-size      # 0x04000000 (64 MiB)
fastboot getvar partition-size:system  # 0x80000000
```

Flash the four partitions, using the **Android names**, then reboot:
```bash
fastboot flash cache    fedora_esp.img          # CAC  -> /boot/efi  (FAT32, 256 MiB)
fastboot flash system   _boot_full.simg         # APP  -> /boot      (ext4, 2.0 GiB)
fastboot flash userdata fedora_uda_full.simg    # UDA  -> / and /home (btrfs, 9400M)
fastboot flash boot     u-boot-baracus.img      # LNX  -> U-Boot (v0 header)
fastboot reboot
```

> **Verify the result before debugging anything on screen.** Boot LineageOS Recovery (**Advanced → Enable ADB**) and compare the flashed bytes. This proved the cache/system/U-Boot writes byte-perfect even while the panel looked dead:
> ```bash
> adb root
> IMG=u-boot-baracus.img
> adb shell sha256sum /dev/block/by-name/CAC            # vs sha256sum fedora_esp.img
> adb exec-out dd if=/dev/block/by-name/LNX bs=4096 count=256 2>/dev/null \
>   | head -c "$(stat -c %s "$IMG")" | sha256sum         # vs sha256sum "$IMG"
> adb shell "dd if=/dev/block/by-name/UDA bs=4096 skip=16 count=1 2>/dev/null | hexdump -C"  # btrfs fsid at +0x20
> ```
> Partition numbers are not stable across kernels: Recovery calls the eMMC `mmcblk0`, Fedora calls it `mmcblk1`.

> **If a write fails, the device is wedged.** After any `Partition write failed!`, every later write fails, including small ones. Recover and retry:
> ```bash
> fastboot reboot-bootloader
> sleep 6
> fastboot devices
> ```
> `fastboot reboot` may print `FAILED (remote: 'unknown reason')`. The unit still reboots.
>
> Expect: `cboot` -> U-Boot (`LNX`) -> `fatload mmc 0:14` -> `booti`. First boot runs the one-time SELinux `.autorelabel` pass and reboots once (about 60–90 s extra). This U-Boot build has **no display output**, so the panel keeps showing the Jamboard logo until the kernel takes over; a "hang on the logo" alone does not prove a boot failure.

---

### Step 2.5: Post-Install GNOME Touch, Storage, & Whiteboard Tuning
1. Complete **GNOME Initial Setup** on the 55" 4K display.
2. **Claim the Remaining ~2.3 GiB of the `UDA` Partition** (the image is 9400M, the partition is 11.33 GiB):
   ```bash
   sudo btrfs filesystem resize max /
   df -h /
   sudo compsize / 2>/dev/null || sudo btrfs filesystem usage /
   ```
3. **Verify the hardware units** before tuning the desktop:
   ```bash
   systemctl status jamboard-touch-init jamboard-pen-daemon jamboard-backlight
   jamboard-scaler get-brightness
   gpioinfo gpiochip0 | sed -n '170,190p'   # confirm lines 174/184/185 are on this chip
   sudo jamboard-scaler input internal   # the panel must now show the Tegra output
   sudo jamboard-pen-daemon --verbose   # then draw with finger, stylus and eraser
   ```
4. Open **Settings → Displays** and set **Scale** to **200%** (ideal touch target sizing for 55" 3840x2160).
5. Open **Settings → Accessibility → Typing** and toggle **Screen Keyboard** ON so the Wayland OSK pops up even when a USB wireless dongle is connected.
6. Install native Wayland touch/stylus whiteboard apps via `dnf` (prefer RPM packages over Flatpaks on the internal eMMC to avoid duplicate 1–2 GB Flatpak runtime downloads!):
   ```bash
   sudo dnf install -y rnote xournalpp
   ```

> **Known open items on this unit (2026-11-17):** the root filesystem mounts and dracut reaches `initrd-root-fs.target`, but the panel output and the USB LAN are not yet confirmed on a fully booted system. `nouveau` firmware and `lp855x`/`ina3221` I2C probes fail *inside the trimmed initramfs* (`lp855x` is the panel backlight chip; the Jamboard normally drives brightness over the scaler UART). Treat "boots to GDM with working touch" as unverified until it is measured on the unit. One measured positive: scaler UART brightness packets **do** reach the panel (the backlight visibly turns on when they are sent), so a dark screen with a working backlight points at the Tegra display path (`tegra-drm`, `sor@54580000`, mode selection), not at the scaler UART.

---

## Common Pitfalls

- **Using the GPT names `CAC`/`APP`/`UDA` with `cboot`:** `cboot` fastboot only knows `cache`, `system`, `userdata`, `boot`, `recovery`, `dtb`. Uppercase names fail with *"No partition present with this name"*.
- **Flashing an `img2simg` image or any sparse image with `FILL`/`DONTCARE` chunks:** the last part fails with `Partition write failed!`, and `DONTCARE` leaves stale Android data in place. Use byte-exact `RAW`-only sparse images (Step 2.3 G).
- **Retrying a write after `Partition write failed!`:** `cboot` stays wedged and rejects every later write. Run `fastboot reboot-bootloader`, wait ~6 s, then retry.
- **Shipping a U-Boot `boot.img` with header v1 or without a ramdisk:** `cboot` silently ignores it and the panel stays on the logo. Use header v0 with a dummy ramdisk (Step 2.1).
- **Assuming the ext4 or EFI/GRUB path is broken:** that was concluded from a scan that never reached `APP` (hex partition numbers) and from a GRUB test whose BLS entry still pointed at the EFI zboot `vmlinuz`. Neither path is verified either way. The FAT ESP + `booti` + plain `Image` path is the one that is verified.
- **Looping over `mmc 0:1..12` in U-Boot:** partition numbers are parsed as **hex**; the ESP is `mmc 0:14` and `/boot` is `mmc 0:13`.
- **Expecting the PCIe `r8169` NIC to work:** there is no PCIe bus. The RJ-45 is a USB `r8152` device behind `xhci-tegra`; load `phy-tegra-xusb`, `xhci-tegra`, `usbnet` and `r8152`.
- **Plugging into the side USB-C port for Fastboot/ADB:** The side USB-C port is wired exclusively to the MStar display scaler as a DisplayPort video input. Always use the **rear Micro-USB service port** for Fastboot and wired ADB.
- **Formatting `/system` before pulling `stock-APP-system.img`:** Once LineageOS is sideloaded, Google's stock `/system` (`APP`) is overwritten. Always pull `APP`, `vendor`, and `RP3` from LineageOS Recovery *immediately* after `fastboot oem unlock` before wiping or sideloading.
- **Flashing `baracus` (Android TV) instead of `baracus_tab` (Tablet) without a Bluetooth remote:** The `baracus` TV build locks initial onboarding behind a Bluetooth remote pairing screen. Use `baracus_tab` for touch/mouse operation.
- **Skipping `.autorelabel`:** Always `touch /.autorelabel` when injecting scripts from another host, or Fedora's Enforcing SELinux policy blocks `jamboard-touch-init.service`.
- **Installing Flatpaks on the internal eMMC:** GNOME/Freedesktop Flatpak runtimes consume 2–4 GiB on their own. Stick to native `dnf` RPM packages (`rnote`, `xournalpp`, `firefox`) and keep `compress=zstd:3` enabled on Btrfs to preserve free working space.
- **Attempting NVIDIA `tegraflash.py` / RCM (APX) mode on a stock Jamboard:** Google blew PKC RSA security fuses on the stock Jetson TX1 module, so unsigned BCT/`cboot` payloads over APX mode are rejected by the BootROM. Always chainload via unlocked `cboot` (`fastboot flash boot ...`), and leave `SOS` (`recovery`) populated with LineageOS Recovery.

## References

- [LineageOS Official Install Guide for Google Jamboard (`baracus`)](https://wiki.lineageos.org/devices/baracus/install/) - Official unlock prerequisites, EoL stock firmware build check, and `cboot` fastboot/recovery workflow.
- [`LineageOS/android_device_google_baracus`](https://github.com/LineageOS/android_device_google_baracus) - Device configuration, `mkbootimg` U-Boot packaging, and `input-port-associations.xml` (`input0`/`input1` RAPT USB mapping).
- [`LineageOS/android_hardware_nvidia_u-boot`](https://github.com/LineageOS/android_hardware_nvidia_u-boot) - Mainline-derived U-Boot fork containing `configs/baracus_defconfig` for chainloading from signed `cboot`.
- [`LineageOS/android_hardware_nvidia_t21x_lineage`](https://github.com/LineageOS/android_hardware_nvidia_t21x_lineage) - Linux 6.x mainline Tegra210 device tree port including `tegra210-baracus.dts` and HazelRed GPIOs.
- [`pb4ugoout/Jamboard`](https://github.com/pb4ugoout/Jamboard) - Hardware teardown, carrier board connector pinouts, and internal subsystem documentation.
- [`TheFitzZZ/jamboard-brightness-baracus`](https://github.com/TheFitzZZ/jamboard-brightness-baracus) - Reverse-engineered 9600-baud `/dev/ttyTHS1` UART brightness packet protocol from stock `scalerdaemon`.

## Related

- [Clean-Room Reverse-Engineering Findings from Stock Google Jamboard ROM](./stock-rom-reverse-engineering-findings.md) - Full findings from `stock-system.img` explaining why unpatched `hid-multitouch` drops the RAPT vendor fields and how stock Jamboard handled erasers and palms.
- [Live Input Capture Findings (LineageOS)](./live-input-capture-findings.md) - Measured RAPT firmware tool classes, extended HID interface, and daemon design inputs.
- [RAPT Optical Touch/Stylus HID & MStar Scaler UART Protocol Guide](./rapt-touch-and-scaler-protocol.md) - Byte-level HID Report `0xF1` and `/dev/ttyTHS1` scaler protocol specs + daemon configuration.
- [`artifacts/jamboard/`](../../../artifacts/jamboard/README.md) - Ready-to-use `rapt-live-probe.sh`, `jamboard-pen-daemon.c`, `jamboard-scaler.sh`, `systemd` units, and the scaler-UART `udev` rule.
