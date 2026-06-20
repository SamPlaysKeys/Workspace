---
type: Note
---

# NVIDIA Container Toolkit & CDI Setup on Proxmox GPU Passthrough VMs

## Overview

NVIDIA Container Toolkit with CDI (Container Device Interface) enables Docker containers to access physical GPUs. On Proxmox VMs with GPU PCI passthrough, CDI specs must be explicitly generated and Docker must be configured to use them.

## Prerequisites

- Proxmox VM with NVIDIA GPU PCI passthrough configured (IOMMU, vfio-pci)
- NVIDIA driver installed and `nvidia-smi` working on the host
- Docker installed

## Step-by-Step

### 1. Install NVIDIA Container Toolkit

```bash
curl -fsSL https://nvidia.github.io/libnvidia-container/gpgkey | sudo gpg --dearmor -o /usr/share/keyrings/nvidia-container-toolkit-keyring.gpg
curl -s -L https://nvidia.github.io/libnvidia-container/stable/deb/nvidia-container-toolkit.list | \
  sed 's#deb https://#deb [signed-by=/usr/share/keyrings/nvidia-container-toolkit-keyring.gpg] https://#g' | \
  sudo tee /etc/apt/sources.list.d/nvidia-container-toolkit.list
sudo apt-get update
sudo apt-get install -y nvidia-container-toolkit
```

### 2. Generate CDI Specs

```bash
sudo nvidia-ctk cdi generate --output=/etc/cdi/nvidia.yaml
```

Verify the spec was created:

```bash
ls -la /etc/cdi/nvidia.yaml
```

### 3. (If Needed) Create AMD CDI Stub

On Proxmox GPU passthrough VMs, Docker's CDI resolution may also require an `amd.yaml` spec. If NemoClaw or other GPU-passthrough tools fail with "AMD CDI spec not found", create a minimal stub:

```bash
sudo tee /etc/cdi/amd.yaml > /dev/null <<'EOF'
---
cdiVersion: "0.5.0"
kind: "amd.com/gpu"
devices:
  - name: "stub"
    containerEdits:
      env:
        - ROCR_VISIBLE_DEVICES = ""
EOF
```

### 4. Configure Docker for CDI

```bash
sudo nvidia-ctk runtime configure --runtime=docker
sudo systemctl restart docker
```

### 5. Verify GPU Access

```bash
docker run --rm --gpus all nvidia/cuda:12.0-base nvidia-smi
```

You should see the GPU listed with driver version and CUDA capabilities.

## Common Pitfalls

| Pitfall | Resolution |
|---------|-----------|
| "AMD CDI spec not found" | Create stub `/etc/cdi/amd.yaml` as described above |
| CDI specs in `/run/cdi/` conflict with `/etc/cdi/` | Clean both directories and regenerate |
| GPU works on host but not in containers | Verify CDI specs exist and Docker was restarted |
| Container starts but `nvidia-smi` fails inside | Check container runtime is `docker` (not containerd directly) |

## References

- [NVIDIA Container Toolkit Installation Guide](https://docs.nvidia.com/datacenter/cloud-native/container-toolkit/latest/install-guide.html)
- [NVIDIA CDI Support](https://docs.nvidia.com/datacenter/cloud-native/container-toolkit/latest/cdi-support.html)
- [OpenShell GPU Passthrough](https://openshell.dev/docs/gpu)
