---
type: Troubleshooting
status: Active
system: Podman
related_to:
  - docs/guides/dev-environment/podman-desktop-macos.md
  - docs/guides/dev-environment/vscode.md
references:
  - https://github.com/containers/krunkit
  - https://github.com/containers/krunkit/blob/main/docs/usage.md
category: Guides
title: Fixing Podman Start issues with Krunkit on MacOS
---

# Fixing Podman Machine Start Failure (krunkit abort trap) on macOS

Podman on macOS can run its Linux VM through **libkrun** via the `krunkit` binary (installed from Homebrew). When required shared libraries are missing, `krunkit` crashes immediately and Podman reports an unhelpful abort-trap error instead of the underlying dyld failure.

This guide documents the symptoms, troubleshooting steps, fix, and prevention for that failure mode.

---

## Symptoms

Starting a libkrun-backed Podman machine fails:

```bash
podman machine start
```

Example output:

```text
Starting machine "podman-machine-default"
Error: krunkit was terminated by signal: abort trap
```

Related indicators:

- `podman machine list` shows the machine in **stopped** state with **Never** under Last Up
- `podman version` fails with a socket/SSH connection error because the VM never started
- The machine was created with VM type **libkrun** (default when `krunkit` is installed)

```bash
podman machine list
# NAME                     VM TYPE     ...
# podman-machine-default*  libkrun     ...  Never
```

---

## Root Cause

`krunkit` depends on a chain of Homebrew libraries (`libkrun-efi`, `virglrenderer`, and others). If any link in that chain is missing, macOS dynamic linker (`dyld`) aborts the process before Podman can surface a clear error.

In this case, two libraries were not installed:

| Missing library | Required by | Symptom when running `krunkit --version` |
|-----------------|-------------|--------------------------------------------|
| `libepoxy` | `libkrun-efi` | `Library not loaded: .../libepoxy.0.dylib` |
| `molten-vk` | `virglrenderer` | `Library not loaded: .../libMoltenVK.dylib` |

Podman only reported the generic **abort trap** because `krunkit` exited before the VM could boot.

---

## Troubleshooting

Work through these steps to isolate the failure.

### 1. Confirm machine state and provider

```bash
podman machine list
podman machine inspect podman-machine-default
```

Look for `VM TYPE: libkrun` and `State: stopped`.

### 2. Run krunkit directly

This usually reveals the real dyld error:

```bash
which krunkit
krunkit --version
```

If `krunkit` crashes with `Library not loaded`, note the missing `.dylib` path.

### 3. Check Homebrew dependency tree

```bash
brew deps krunkit
brew deps --installed krunkit libkrun-efi
```

Compare installed deps against what `krunkit --version` reports as missing.

### 4. Inspect linked libraries (optional)

```bash
otool -L "$(which krunkit)"
```

Follow the chain into `/opt/homebrew/Cellar/` if multiple libraries are missing.

---

## Resolution

Install the missing libraries reported by `krunkit --version`, then start the machine.

For the missing `libepoxy` and `molten-vk` libraries:

```bash
brew install libepoxy molten-vk
```

Verify `krunkit` runs:

```bash
krunkit --version
# krunkit 1.1.1
```

Start the Podman machine:

```bash
podman machine start
```

If dependencies are still broken or versions are mismatched, reinstall the krunkit stack so Homebrew relinks everything:

```bash
brew reinstall krunkit libkrun-efi
podman machine start
```

---

## Verification

Confirm client–server connectivity:

```bash
podman version
```

Run a test container:

```bash
podman run --rm hello-world
```

Expected: image pulls successfully and prints the Podman hello-world banner.

---

## Prevention

- **Install krunkit from the maintained tap** so Homebrew tracks dependencies:

  ```bash
  brew tap slp/krun
  brew install krunkit
  ```

- **Avoid orphaning dependencies** — if you run `brew cleanup` or manually remove packages, check that `krunkit --version` still works before starting Podman.

- **After major macOS or Homebrew upgrades**, verify the stack:

  ```bash
  krunkit --version && podman machine start
  ```

- **Rootless vs rootful** — the default machine runs rootless. If you need ports below 1024 or hit Docker-client compatibility issues:

  ```bash
  podman machine set --rootful
  ```

---

## Environment (reference)

This issue was diagnosed on:

| Item | Value |
|------|-------|
| OS | macOS 26.5.1 (arm64) |
| Podman | 5.8.2 (Homebrew) |
| krunkit | 1.1.1 |
| Machine provider | libkrun |

---

## References

- [containers/krunkit](https://github.com/containers/krunkit) — libkrun VM launcher for macOS; Homebrew install instructions
- [krunkit usage docs](https://github.com/containers/krunkit/blob/main/docs/usage.md) — VM configuration and device options
