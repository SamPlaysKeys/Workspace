---
type: Guide
status: Active
system: Podman Desktop
related_to:
  - docs/guides/dev-environment/podman-machine-krunkit-abort-trap.md
  - docs/guides/dev-environment/vscode.md
references:
  - https://podman-desktop.io/docs/installation/macos-install
  - https://podman-desktop.io/docs/migrating-from-docker/managing-docker-compatibility
  - https://podman.io/docs/installation
  - https://github.com/containers/krunkit
---

# Podman Desktop Setup on macOS

Podman Desktop is the graphical front end for Podman on macOS. Containers do not run natively on macOS — Podman launches a lightweight Linux VM (a **Podman machine**) and runs containers inside it. The desktop app handles onboarding, machine management, and optional Docker compatibility so existing Docker workflows keep working.

This guide covers installation, first-run setup, Docker compatibility, rootless vs rootful mode, and common gotchas — including the **krunkit abort trap** failure on Homebrew installs.

---

## Prerequisites

| Requirement | Notes |
|-------------|-------|
| macOS 14+ | Required for libkrun/krunkit on Apple Silicon |
| Apple Silicon or Intel | Universal `.dmg` works on both |
| Admin password | Needed during first-run Podman engine install |
| ~4 GB RAM minimum | Default machine uses 2 GiB; increase for heavier workloads |

Optional but useful:

- **Homebrew** — alternative install path (see gotchas below)
- **Docker CLI** — not required; Podman can serve the Docker API. Install via onboarding or `brew install docker` if you want the `docker` command

---

## Choose an Install Path (Important)

Podman Desktop and the Podman CLI can be installed two ways. **Pick one and stay on it.**

| Method | Command / source | Official stance |
|--------|-------------------|-----------------|
| **DMG installer (recommended)** | [podman-desktop.io/downloads/macos](https://podman-desktop.io/downloads/macos) | Most stable; bundles Podman, krunkit, and CLIs |
| **Homebrew** | `brew install --cask podman-desktop` | Supported but not recommended by upstream |

### Do not mix install methods

If Podman is already installed via Homebrew, either:

- **Uninstall Homebrew Podman** before using the `.dmg` installer, or
- **Use Homebrew only** — do not also install the `.dmg`

Mixing paths causes duplicate binaries, conflicting machine configs, and hard-to-debug startup failures.

```bash
# Check what you already have
which podman
podman --version
brew list --cask podman-desktop 2>/dev/null
ls /Applications/Podman\ Desktop.app 2>/dev/null
```

---

## Installation

### Option A: DMG installer (recommended)

1. Download the **Universal** `.dmg` from [Podman Desktop macOS downloads](https://podman-desktop.io/downloads/macos).
2. Open the `.dmg` and drag **Podman Desktop** to **Applications**.
3. Launch from Applications or Spotlight (`Cmd+Space` → "Podman Desktop").
4. Complete onboarding (see [First-run setup](#first-run-setup) below).

The installer bundles krunkit and avoids the Homebrew dependency-chain issues described in [Podman Machine krunkit Abort Trap](./podman-machine-krunkit-abort-trap.md).

### Option B: Homebrew

```bash
brew install --cask podman-desktop
```

Homebrew installs Podman Desktop and the Podman engine if not already present. Open the app from `/Applications`.

If you use libkrun (krunkit is on your `PATH`), install krunkit from the maintained tap so dependencies are tracked:

```bash
brew tap slp/krun
brew install krunkit
```

After any Homebrew install, verify krunkit before creating a machine:

```bash
krunkit --version
```

If this crashes, see the [krunkit abort trap guide](./podman-machine-krunkit-abort-trap.md) before proceeding.

---

## First-run setup

Podman Desktop onboarding walks through engine install, machine creation, and optional CLIs. You can also finish setup later from **Settings → Resources** or the Dashboard notification.

### Onboarding checklist

1. **Install Podman engine** — enter your macOS password when prompted.
2. **Create a Podman machine** — default name is `podman-machine-default`.
3. **Install optional CLIs** — `kubectl` and `compose` (Podman Compose).
4. **Open Dashboard** — confirm the machine shows **Running** under **Settings → Resources**.

### CLI equivalent

If you prefer the terminal (or need to script setup):

```bash
podman machine init
podman machine start
podman info
podman run --rm hello-world
```

Customize resources before or after init:

```bash
podman machine set --cpus 6 --memory 4096 --disk-size 100
podman machine start
```

### Machine providers

On Apple Silicon with krunkit installed, new machines default to **libkrun** (GPU-capable, uses Apple's Hypervisor framework). Without krunkit, Podman falls back to **applehv**.

```bash
podman machine list
# VM TYPE column: libkrun or applehv
```

---

## Rootless vs rootful

Every Podman machine runs containers in one of two modes. **Rootless is the default.**

| | Rootless (default) | Rootful |
|---|-------------------|---------|
| **Container UID mapping** | Runs as your user inside the VM | Runs as root inside the VM |
| **Security** | Smaller blast radius | Broader privileges in the VM |
| **Privileged ports (< 1024)** | Cannot bind (e.g. port 80, 443) | Can bind |
| **Some Docker tools** | May need extra socket/config | Better compatibility |
| **Editing VM system files** | Requires `podman machine ssh` as user | Easier access to `/etc/containers/` |

### When to stay rootless

- General development and learning
- Running apps on high ports (3000, 8080, etc.)
- You want the safer default

### When to switch to rootful

- Binding ports below 1024
- Tools that assume root-owned containers or the root Podman socket
- Permission errors editing registries or storage inside the VM

### How to switch

**Podman Desktop:** Settings → Resources → select your machine → toggle rootful mode, then restart the machine.

**CLI:**

```bash
podman machine stop
podman machine set --rootful
podman machine start
```

To revert:

```bash
podman machine stop
podman machine set --rootful=false
podman machine start
```

Podman maintains separate connection endpoints for each mode:

```bash
podman system connection list
# podman-machine-default       → rootless socket
# podman-machine-default-root  → rootful socket
```

Switch the active connection if needed:

```bash
podman system connection default podman-machine-default-root
```

---

## Docker compatibility

Podman exposes a **Docker-compatible API** so tools and scripts written for Docker can talk to Podman instead. On macOS, Podman Desktop enables **Third-Party Docker Tool Compatibility** by default.

### What gets mapped

When the machine is running, macOS exposes a system socket symlink:

```bash
ls -la /var/run/docker.sock
# → ~/.local/share/containers/podman/machine/podman.sock
```

Podman Desktop reports socket status under **Settings → Docker Compatibility**.

### Using the Docker CLI with Podman

1. Ensure the Podman machine is running.
2. Enable Docker compatibility in Podman Desktop (on by default on macOS).
3. Install the Docker CLI if you do not have it (onboarding or `brew install docker`).
4. Verify:

```bash
docker info --format '{{.ServerVersion}}'
# Should report a Podman version, not Docker Desktop
```

### DOCKER_HOST (explicit configuration)

Some tools (Gradle, Testcontainers, CI scripts) need an explicit socket path instead of relying on `/var/run/docker.sock`:

```bash
export DOCKER_HOST="unix://${HOME}/.local/share/containers/podman/machine/podman.sock"
```

Or create a Docker context:

```bash
docker context create podman \
  --docker "host=unix://${HOME}/.local/share/containers/podman/machine/podman.sock"
docker context use podman
```

### Compose

- **Podman Compose** — installed during onboarding; invoke as `podman compose` (preferred on Podman).
- **Docker Compose** — works against the Docker-compatible socket when compatibility is enabled.

```bash
podman compose version
podman compose up -d
```

Compose project naming and networking differ slightly from Docker Compose in edge cases; prefer `podman compose` for Podman-native behavior.

### Coexistence with Docker Desktop

Do **not** run Docker Desktop and Podman Desktop socket forwarding at the same time — both compete for `/var/run/docker.sock`. Quit Docker Desktop (or disable its socket) before enabling Podman Docker compatibility.

---

## Gotchas

### 1. krunkit abort trap (Homebrew + libkrun)

**Symptom:** `podman machine start` fails with `krunkit was terminated by signal: abort trap`.

**Cause:** Missing Homebrew libraries in the krunkit dependency chain (`libepoxy`, `molten-vk`, etc.).

**Fix:** See [Podman Machine krunkit Abort Trap](./podman-machine-krunkit-abort-trap.md).

Quick check:

```bash
krunkit --version   # should print a version, not crash
```

### 2. Mixed Homebrew and DMG installs

Installing Podman via Homebrew and Podman Desktop via `.dmg` (or vice versa) leaves duplicate `podman`/`krunkit` binaries and conflicting machine state. Uninstall one path completely before adopting the other.

### 3. krunkit not found

Podman Desktop onboarding or `podman machine init` may report that krunkit is missing when using a partial Homebrew install.

```bash
brew tap slp/krun
brew install krunkit
```

The official `.dmg` bundles krunkit and avoids this.

### 4. Rootless privileged ports

Binding host port 80 in rootless mode fails silently or with permission errors. Switch to rootful or use a high port (8080) with a reverse proxy.

### 5. ARM64 image platform

On Apple Silicon, prefer multi-arch images or specify the platform:

```bash
podman run --platform linux/arm64 ...
```

Running x86-only images requires emulation and is slower.

### 6. Docker CLI still talks to Docker Desktop

If `docker info` shows Docker Desktop after switching to Podman:

- Quit Docker Desktop
- Confirm Podman machine is running
- Check **Settings → Docker Compatibility** in Podman Desktop
- Run `docker context ls` and switch away from `desktop-linux`

### 7. Machine resource defaults

The default 2 GiB RAM is tight for multiple containers or image builds. Increase before heavy use:

```bash
podman machine set --memory 8192 --cpus 4
podman machine start
```

Changes apply on next start.

### 8. Registries and VM config edits

Files like `/etc/containers/registries.conf` live **inside the VM**, not on macOS. Edit from the host:

```bash
podman machine ssh
# or, if permission denied in rootless mode:
podman machine set --rootful && podman machine start
```

---

## Verification

Run this checklist after setup:

```bash
# Machine running
podman machine list

# Client ↔ VM connectivity
podman version

# Basic container
podman run --rm hello-world

# Docker API socket (when compatibility enabled)
ls -la /var/run/docker.sock

# Optional: Docker CLI against Podman
docker info --format '{{.ServerVersion}}'
```

In Podman Desktop: **Settings → Resources** shows the machine as **Running**; **Settings → Docker Compatibility** shows the socket as reachable.

---

## Quick reference

| Task | Command / location |
|------|-------------------|
| Start machine | `podman machine start` or Podman Desktop Dashboard |
| Stop machine | `podman machine stop` |
| SSH into VM | `podman machine ssh` |
| Enable rootful | `podman machine set --rootful` |
| Docker compatibility | Settings → Docker Compatibility |
| Fix krunkit crash | [krunkit abort trap guide](./podman-machine-krunkit-abort-trap.md) |
| Increase RAM/CPUs | `podman machine set --memory 8192 --cpus 4` |

---

## References

- [Podman Desktop macOS install](https://podman-desktop.io/docs/installation/macos-install) — official install and onboarding
- [Managing Docker compatibility](https://podman-desktop.io/docs/migrating-from-docker/managing-docker-compatibility) — socket mapping and CLI setup
- [Podman installation (macOS)](https://podman.io/docs/installation) — CLI and machine overview
- [containers/krunkit](https://github.com/containers/krunkit) — libkrun VM backend for Apple Silicon
