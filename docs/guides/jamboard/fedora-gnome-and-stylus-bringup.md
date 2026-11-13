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
2. **Stage 2 (Fedora Workstation `aarch64` Bring-Up on Internal eMMC):** Chainload a UEFI-enabled U-Boot from Google's signed `cboot` bootloader, deploy stock Fedora Workstation (GNOME Wayland) **100% onto the internal 16 GB eMMC** (`CAC` for `/boot/efi`, `APP` for `/boot`, and `UDA` with `zstd` Btrfs compression for `/`), and enable the Jamboard hardware daemons (touchscreen GPIO init, UART scaler brightness, and the pen/eraser translator).

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
  - 100–120 Hz infrared optical waveguide bezel driven by an internal NXP/RAPT Touch controller (`VID:PID 1cbe:0003`) wired to internal USB host port **`usb-70090000.xusb-4`** (`usb2-3` UTMI).
  - **Passive Stylus & Eraser:** Neither accessory contains electronics or batteries. The bezel measures the physical 2D optical shadow/ellipse on the glass (~1.5–3 mm fine stylus tip, ~8–15 mm finger, ~40–60 mm eraser puck).
  - **Power/Boot Sequence Required:** On every boot, the host must toggle three Tegra GPIOs (`pwren=174` / `V6`, `bootmode=184` / `X0`, `reset=185` / `X1`) to boot the touch MCU from SPIFI flash before it enumerates on USB.

### Physical Port & Button Identification (For Hands-On Operator)

| Port / Control | Location | Function | Important Notes |
| :--- | :--- | :--- | :--- |
| **Micro-USB Service Port** | **Rear / Back I/O bay** | **Fastboot & USB ADB** | Wired directly to Tegra OTG (`usb@7d000000`). **Use this port for flashing and wired ADB.** |
| **USB-C Port** | Side bezel | DisplayPort Video-In ONLY | **Cannot** be used for Fastboot or ADB. |
| **2x USB 3.0 Type-A** | Side bezel | Host USB (Keyboard, Mouse, SSD) | Driven by `tegra-xusb` (`xusb@70090000`). Active once Linux loads `nvidia/tegra210/xusb.bin`. |
| **RJ-45 Gigabit Ethernet** | Rear I/O bay | PCIe Realtek `RTL8169` | Works in both U-Boot and Linux (`r8169`). |
| **Power / Vol+ / Vol-** | Side/Rear edge | Power & Bootloader navigation | Hold **Power for 4 seconds** from powered-off state to enter `cboot` Fastboot menu. |

### Internal 16 GB eMMC Partition Map (`sdhci-tegra.3`)

| Partition Name | Android Mount | Size | Role in LineageOS (Stage 1) | Role in Fedora GNOME (Stage 2) |
| :--- | :--- | :--- | :--- | :--- |
| `LNX` (`boot`) | `/boot` | 25.5 MiB | U-Boot payload booting Android | UEFI-enabled U-Boot (`u-boot-baracus.img`) |
| `SOS` (`recovery`) | `/recovery` | 25.5 MiB | LineageOS Recovery (Root ADB) | **Keep LineageOS Recovery** as permanent rescue tool |
| `DTB` (`dtb`) | *(raw)* | ~4 MiB | Bootloader/CBoot DTB | Keep LineageOS `dtb.img` |
| `CAC` (`cache`) | `/cache` | **256 MiB** | Android cache | **Fedora `/boot/efi`** (`vfat` ESP) |
| `APP` (`system`) | `/system` | **2.0 GiB** | LineageOS `/system` | **Fedora `/boot`** (`ext4`: `vmlinuz`, `initramfs`, `dtb`) |
| `vendor` | `/vendor` | ~540 MiB | Proprietary blobs | **Do not wipe** (preserve reference blobs) |
| `RP3` | `/mnt/vendor/wifi_config` | Small `ext4` | Per-unit Wi-Fi MAC & RF trim | **NEVER WIPE** (unit-unique calibration) |
| `UDA` (`userdata`) | `/data` | **9.4 GiB** | Android `/data` | **Fedora `/` + `/home`** (`btrfs`, `compress=zstd:3`) |

---

## Prerequisites

### Hands-On Operator Workstation & Gear
- Linux or macOS laptop with current `android-platform-tools` (`adb`, `fastboot`, and `img2simg` / `android-tools`) plus a Linux environment (native or VM) with `btrfs-progs`, `dosfstools`, `rsync`, and `losetup` for Stage 2 image assembly.
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
- The RAPT Touch USB device (`1cbe:0003` at `usb-70090000.xusb-4`) exports large USB HID Report Descriptors (2,313 B and 3,149 B) and **multiple USB HID interfaces** (`input0` and `input1`).
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
| *(UEFI Firmware)* | Android `boot.img` | `LNX` (`boot`) | 25.5 MiB | Chainloads UEFI-enabled U-Boot from stock `cboot` |
| *(Rescue OS)* | Android `boot.img` | `SOS` (`recovery`) | 25.5 MiB | **Keeps LineageOS Recovery** for permanent Root ADB rescue |
| `/boot/efi` | `vfat` (FAT32) | `CAC` (`cache`) | **256 MiB** | Fedora's EFI binaries + DTB use ~25 MiB |
| `/boot` | `ext4` | `APP` (`system`) | **2.0 GiB** | Exact 1:1 size match with Fedora's raw disk `/boot` partition |
| `/` & `/home` | `btrfs` (`zstd:3`) | `UDA` (`userdata`) | **9.40 GiB** (`10,099,646,976` B) | Fresh Fedora Workstation compresses from ~4.8 GiB down to **~2.8 GiB**, leaving **~6.6 GiB free** |

---

### Step 2.1: Build UEFI-Enabled U-Boot (`u-boot-baracus.img`)
Google's Jetson TX1 SoC has PKC security fuses blown, preventing raw RCM/APX bootloader replacement, **but** unlocked stock `cboot` happily boots any unsigned Android `boot.img` from `LNX`. LineageOS already proved mainline-derived U-Boot chainloading from `cboot` (`LineageOS/android_hardware_nvidia_u-boot`).

On an `x86_64` or `aarch64` Linux build machine:
```bash
git clone https://github.com/LineageOS/android_hardware_nvidia_u-boot.git -b lineage-22.2
cd android_hardware_nvidia_u-boot

make ARCH=arm CROSS_COMPILE=aarch64-linux-gnu- baracus_defconfig
```
Append/enable the standard U-Boot UEFI (`efi_loader` + `bootstd`) options in `.config` (then run `make ARCH=arm CROSS_COMPILE=aarch64-linux-gnu- olddefconfig`):
```ini
CONFIG_EFI_LOADER=y
CONFIG_CMD_BOOTEFI=y
CONFIG_BOOTSTD=y
CONFIG_BOOTSTD_DEFAULTS=y
CONFIG_BOOTMETH_EFI=y
CONFIG_FS_FAT=y
CONFIG_FS_EXT4=y
CONFIG_EFI_PARTITION=y
CONFIG_OF_BOARD_SETUP=y
CONFIG_OF_SYSTEM_SETUP=y
CONFIG_BOOTCOMMAND="setenv fdtfile nvidia/tegra210-baracus.dtb; bootflow scan -lb"
```
> **Note on `fdtfile`:** Having U-Boot load `/dtb/nvidia/tegra210-baracus.dtb` *before* `bootefi` ensures U-Boot runs `ft_system_setup()` to copy `cboot`'s `/cpus` and `/psci` SMP nodes and memory carveouts into the DTB handed to UEFI/GRUB.

Compile and wrap `u-boot-dtb.bin` into a header-v1 Android boot image:
```bash
make -j"$(nproc)" ARCH=arm CROSS_COMPILE=aarch64-linux-gnu-
mkbootimg --kernel u-boot-dtb.bin --header_version 1 -o u-boot-baracus.img
```

---

### Step 2.2: Compile Mainline Device Tree (`tegra210-baracus.dtb`)
Stock Fedora `aarch64` `kernel-core` already enables `ARCH_TEGRA_210_SOC`, `tegra-drm`, `nouveau`, `xhci-tegra`, `sdhci-tegra`, `r8169`, `brcmfmac`, and `serial-tegra`, and ships Jetson TX1's `tegra210-p2371-2180.dtb`. Build `tegra210-baracus.dtb` (adapting `LineageOS/android_hardware_nvidia_t21x_lineage`'s `tegra210-baracus.dts` onto mainline `tegra210-p2371-2180.dts`) with these four Jamboard overrides:
1. **HDMI (`sor1` at `hdmi@54280000`):** `status = "okay"` with `avdd_hdmi-supply = <&max77620_ldo8>` (`regulator-always-on`); keep `dsi@54300000` disabled.
2. **Internal Touch USB (`xusb@70090000` / `padctl@7009f000`):** Enable UTMI port `usb2-3` in host mode for the RAPT Touch controller.
3. **Scaler UART (`serial@70006040`):** Ensure UARTB (`/dev/ttyTHS1`) is `status = "okay"`.
4. **TrustZone / PMC Compatibility:** Omit `nvidia,secure-pmc` on `pmc@7000e400` if stock TZ monitor traps secure PMC writes.

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
Preserve the exact FAT32 volume ID (`UUID`) from `${LOOP}p1` so `/etc/fstab` needs no UUID edits:
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
sudo umount /tmp/esp-src /tmp/esp-dst
```

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

#### 3. Build the 9.4 GiB Internal eMMC Rootfs Image (`fedora_uda.simg` for `UDA`)
Because Fedora's raw `${LOOP}p3` partition is ~10.3 GiB while the Jamboard's internal `UDA` partition is **`10,099,646,976` bytes (`9.40 GiB`)**, we create a `9400M` (`9,856,614,400` bytes) Btrfs image with **the exact same UUID** as `${LOOP}p3`, mount it with **`compress=zstd:3`** so Fedora shrinks to ~2.8 GiB on disk, copy the `root` and `home` subvolumes into it, and inject the Jamboard hardware services:

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

- **A. Enable `zstd:3` Btrfs Compression & 9.4 GiB eMMC Space Hygiene:**
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
  Create `$FED_ROOT/etc/dracut.conf.d/jamboard.conf`:
  ```conf
  add_drivers+=" mmc_block sdhci-tegra phy-tegra-xusb xhci-tegra "
  install_items+=" /usr/lib/firmware/nvidia/tegra210/xusb.bin "
  ```
  *(Inside an `aarch64` `systemd-nspawn` / `chroot` with `qemu-user-static`, run `dnf install -y libgpiod-utils && dracut --force --regenerate-all`—or run `dnf install -y libgpiod-utils` over Ethernet/USB keyboard right after first boot).*

- **C. Broadcom 802.11ac Wi-Fi NVRAM:**
  Copy the `bcm4354` NVRAM file extracted from `stock-vendor.img` / `stock-RP3-wifi.img` (or NVIDIA's `nvram_jetsonE_cv_4354.txt`) to:
  `$FED_ROOT/usr/lib/firmware/brcm/brcmfmac4354-sdio.txt`

- **D. Touchscreen GPIO Wake-Up Script & Service (`/usr/local/bin/jamboard-touch-init`):**
  ```bash
  sudo tee "$FED_ROOT/usr/local/bin/jamboard-touch-init" > /dev/null << 'EOF'
  #!/usr/bin/bash
  set -euo pipefail
  # Assert reset (185=1), select SPIFI boot (184=0), enable step-down power active-low (174=0)
  gpioset -c gpiochip0 185=1 184=0 174=0 &
  GP_PID=$!
  sleep 0.25
  kill "$GP_PID" 2>/dev/null || true
  # Release reset (185=0) and hold power on (174=0)
  exec gpioset -c gpiochip0 185=0 184=0 174=0
  EOF
  sudo chmod +x "$FED_ROOT/usr/local/bin/jamboard-touch-init"

  sudo tee "$FED_ROOT/etc/systemd/system/jamboard-touch-init.service" > /dev/null << 'EOF'
  [Unit]
  Description=Google Jamboard RAPT Optical Touchscreen SPIFI Power-On
  Before=systemd-udevd.service display-manager.service
  DefaultDependencies=no

  [Service]
  Type=simple
  ExecStart=/usr/local/bin/jamboard-touch-init
  Restart=on-failure

  [Install]
  WantedBy=sysinit.target
  EOF
  sudo ln -sf /etc/systemd/system/jamboard-touch-init.service \
    "$FED_ROOT/etc/systemd/system/sysinit.target.wants/jamboard-touch-init.service"
  ```

- **E. MStar Scaler Display Backlight Script & Boot Default (`/usr/local/bin/jamboard-brightness`):**
  ```bash
  sudo tee "$FED_ROOT/usr/local/bin/jamboard-brightness" > /dev/null << 'EOF'
  #!/usr/bin/bash
  set -euo pipefail
  PCT="${1:-80}"
  if (( PCT < 0 || PCT > 100 )); then
    echo "Usage: jamboard-brightness <0-100>" >&2
    exit 1
  fi
  CHK=$(( PCT ^ 0xE9 ))
  stty -F /dev/ttyTHS1 9600 raw -echo
  printf "\x6e\x51\x86\x03\xf5\xdd\x85\xfe\x$(printf '%02x' "$PCT")\x$(printf '%02x' "$CHK")" > /dev/ttyTHS1
  EOF
  sudo chmod +x "$FED_ROOT/usr/local/bin/jamboard-brightness"

  sudo tee "$FED_ROOT/etc/systemd/system/jamboard-backlight.service" > /dev/null << 'EOF'
  [Unit]
  Description=Set Google Jamboard MStar Scaler Default Brightness
  After=dev-ttyTHS1.device

  [Service]
  Type=oneshot
  ExecStart=/usr/local/bin/jamboard-brightness 80

  [Install]
  WantedBy=multi-user.target
  EOF
  sudo ln -sf /etc/systemd/system/jamboard-backlight.service \
    "$FED_ROOT/etc/systemd/system/multi-user.target.wants/jamboard-backlight.service"
  ```

- **F. Install `jamboard-pen-daemon` (from Stage 1) & Trigger SELinux Relabel:**
  Copy the compiled `aarch64` `jamboard-pen-daemon` binary to `$FED_ROOT/usr/local/bin/jamboard-pen-daemon`, enable its `systemd` unit (`After=jamboard-touch-init.service`), and ensure Fedora's Enforcing SELinux policy properly labels our newly injected files on first boot:
  ```bash
  sudo touch "$FED_ROOT/.autorelabel"
  ```

- **G. Unmount & Convert `fedora_uda_raw.img` to an Android Sparse Image:**
  Because Tegra X1's `cboot` fastboot download buffer is ~256–512 MiB, converting the 9.4 GiB raw image with `img2simg` lets `fastboot` stream it in chunks automatically and skip unused blocks:
  ```bash
  sudo umount /tmp/dst-btrfs
  sudo losetup -d "$LOOP"

  img2simg fedora_uda_raw.img fedora_uda.simg
  ```

---

### Step 2.4: Flash Everything to Internal eMMC via Fastboot

Enter `cboot` Fastboot mode (power off, connect rear Micro-USB, hold **Power** for 4 seconds) and flash all four partitions to the internal 16 GB eMMC:

```bash
fastboot flash CAC  fedora_esp.img        # 256 MiB FAT32 -> /boot/efi
fastboot flash APP  fedora_boot.img       # 2.0 GiB ext4  -> /boot
fastboot flash UDA  fedora_uda.simg       # 9.4 GiB Btrfs -> / and /home (compressed)
fastboot flash boot u-boot-baracus.img    # UEFI-enabled U-Boot -> LNX
fastboot reboot
```
*(Note: First boot will take an extra ~60–90 seconds while Fedora runs the one-time SELinux `.autorelabel` pass and reboots into GDM).*

---

### Step 2.5: Post-Install GNOME Touch, Storage, & Whiteboard Tuning
1. Complete **GNOME Initial Setup** on the 55" 4K display.
2. **Claim the Remaining ~230 MiB of the `UDA` Partition:**
   ```bash
   sudo btrfs filesystem resize max /
   df -h /
   sudo compsize / 2>/dev/null || sudo btrfs filesystem usage /
   ```
3. Open **Settings → Displays** and set **Scale** to **200%** (ideal touch target sizing for 55" 3840x2160).
4. Open **Settings → Accessibility → Typing** and toggle **Screen Keyboard** ON so the Wayland OSK pops up even when a USB wireless dongle is connected.
5. Install native Wayland touch/stylus whiteboard apps via `dnf` (prefer RPM packages over Flatpaks on the 9.4 GiB internal eMMC to avoid duplicate 1–2 GB Flatpak runtime downloads!):
   ```bash
   sudo dnf install -y rnote xournalpp
   ```

---

## Common Pitfalls

- **Plugging into the side USB-C port for Fastboot/ADB:** The side USB-C port is wired exclusively to the MStar display scaler as a DisplayPort video input. Always use the **rear Micro-USB service port** for Fastboot and wired ADB.
- **Formatting `/system` before pulling `stock-APP-system.img`:** Once LineageOS is sideloaded, Google's stock `/system` (`APP`) is overwritten. Always pull `APP`, `vendor`, and `RP3` from LineageOS Recovery *immediately* after `fastboot oem unlock` before wiping or sideloading.
- **Flashing `baracus` (Android TV) instead of `baracus_tab` (Tablet) without a Bluetooth remote:** The `baracus` TV build locks initial onboarding behind a Bluetooth remote pairing screen. Use `baracus_tab` for touch/mouse operation.
- **Flashing an unsparsed 9.4 GiB `UDA` image or skipping `.autorelabel`:** Always convert `fedora_uda_raw.img` with `img2simg` so `cboot` fastboot chunks transfers within its RAM buffer, and always `touch /.autorelabel` when injecting scripts from another host so Fedora's Enforcing SELinux policy doesn't block `jamboard-touch-init.service`.
- **Installing Flatpaks on the 9.4 GiB internal eMMC:** GNOME/Freedesktop Flatpak runtimes consume 2–4 GiB on their own. Stick to native `dnf` RPM packages (`rnote`, `xournalpp`, `firefox`) and keep `compress=zstd:3` enabled on Btrfs to preserve ~5–6 GiB of free working space.
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
- [`artifacts/jamboard/`](../../../artifacts/jamboard/README.md) - Ready-to-use `rapt-live-probe.sh`, `jamboard-pen-daemon.c`, `jamboard-scaler.sh`, and `systemd` service units.
