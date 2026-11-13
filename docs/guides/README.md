---
title: Setup & Prevention Guides
status: Meta
type: Index
system: Workspace Documentation
category: Guides
tags: [guides, index]
date: 2026-06-07
related_to: []
references: []
---

This directory contains guides and walkthroughs for correctly configuring and maintaining systems.

## Topic Index

### OpenBao
- [Tailscale Integration for OpenBao](../architecture/openbao/tailscale-integration.md) - Secure, on-demand generation of Tailscale device auth keys for Docker containers.

### Ansible & AAP
- [Exporting and Enriching AAP Job Templates and Workflows](./ansible/aap-job-template-export.md) - Extract and resolve Job Templates, Workflow Job Templates, Inventories, and Projects into unified CSV/JSON reports via AAP Controller API v2.

### Dev Environment
- [VSCode Setup Guide](./dev-environment/vscode.md) - Configuration instructions for VSCode in enterprise development environments.
- [Podman Desktop on macOS](./dev-environment/podman-desktop-macos.md) - Install Podman Desktop, configure Docker compatibility, and choose rootless vs rootful mode.
- [Podman Machine krunkit Abort Trap](./dev-environment/podman-machine-krunkit-abort-trap.md) - Fix Podman machine start failures on macOS when krunkit crashes due to missing Homebrew libraries.

### GPU / CUDA
- [NVIDIA Container Toolkit & CDI on Proxmox VMs](./gpu/nvidia-container-toolkit-cdi-proxmox.md) - Setup CDI GPU passthrough for Docker containers on Proxmox VMs.

### OpenShift
- [OpenShift Guides Index](./openshift/README.md) - Setup instructions for GPU, internal registries, and cluster prep.

### OpenShift Virtualization
- [OpenShift Virtualization Guides Index](./openshift-virtualization/README.md) - Storage configurations, PVC verification, and VM scheduling/balancing.
- [VM Migration & Upgrade Strategy](./openshift-virtualization/openshift-virtualization-upgrade/README.md) - Minimize downtime during cluster upgrades with live migration tuning, VM classification, and orchestration workflows (Ansible, AAP, ACM).

### HCL BigFix
- [Planning & Architecture Guide](./bigfix/planning.md) - Sizing, network ports, database selection, and discovery checklists for deployment.

### Hardware & Embedded Linux
- [Fedora Workstation (GNOME) & Stylus/Eraser Bring-Up on Google Jamboard](./jamboard/fedora-gnome-and-stylus-bringup.md) - Complete two-stage plan for backing up stock Android 7, reverse-engineering the RAPT optical stylus/eraser via LineageOS ADB, and installing stock Fedora `aarch64` onto the internal 16 GB eMMC.
- [Clean-Room Reverse-Engineering Findings from Stock Google Jamboard ROM](./jamboard/stock-rom-reverse-engineering-findings.md) - Interoperability analysis of `stock-system.img` (`rapt_hid_descriptor.bin`, `libinputflinger.so` `MT_TOOL_ERASER=3`, `JamKiosk` wedge/chisel stroke math, and Google `karma` daemons).
- [RAPT Optical Touch/Stylus HID & MStar Scaler UART Protocol Guide](./jamboard/rapt-touch-and-scaler-protocol.md) - Wire-level specification for RAPT HID Report `0xF1` (340/484-byte interfaces), `uinput` stylus/eraser daemon setup, and MStar `/dev/ttyTHS1` 9600-baud UART brightness and HDMI/USB-C input switching.
- [Live Input Capture Findings for the Google Jamboard RAPT Touch Controller](./jamboard/live-input-capture-findings.md) - Measured RAPT firmware tool classes (stylus/finger/eraser), extended HID interface, palm clustering data, and where it differs from the stock-ROM analysis.


## Frontmatter Convention

Every `.md` file under `docs/guides/` must carry these keys (values may be empty, keys may not be missing):

`type` (`Guide`, `Reference`, `Troubleshooting`, `Design`, `ADR`, `Index`), `status`, `system`, `category`, `title`, `tags`, `date`, `related_to`, `references`.

The repo pre-commit hook (`.githooks/pre-commit` -> `.github/scripts/guides_frontmatter.py`) adds any missing keys to staged guide files and rejects unknown `type` values. Enable it once per clone with `git config --local core.hooksPath .githooks`. Check everything manually with `python3 .github/scripts/guides_frontmatter.py --all`.
