# Setup & Prevention Guides

This directory contains guides and walkthroughs for correctly configuring and maintaining systems.

## Topic Index

### OpenBao
- [Tailscale Integration for OpenBao](./openbao/tailscale-integration.md) - Secure, on-demand generation of Tailscale device auth keys for Docker containers.

### Dev Environment
- [VSCode Setup Guide](./dev-environment/vscode.md) - Configuration instructions for VSCode in enterprise development environments.
- [Podman Desktop on macOS](./dev-environment/podman-desktop-macos.md) - Install Podman Desktop, configure Docker compatibility, and choose rootless vs rootful mode.
- [Podman Machine krunkit Abort Trap](./dev-environment/podman-machine-krunkit-abort-trap.md) - Fix Podman machine start failures on macOS when krunkit crashes due to missing Homebrew libraries.

### GPU / CUDA
- [NVIDIA Container Toolkit & CDI on Proxmox VMs](./gpu/nvidia-container-toolkit-cdi-proxmox.md) - Setup CDI GPU passthrough for Docker containers on Proxmox VMs.

### OpenShift
- [OpenShift Guides Index](./openshift/README.md) - Setup instructions for GPU, internal registries, and cluster prep.
