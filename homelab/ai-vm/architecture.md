---
layout: page
title: AI VM Architecture
category: Homelab
status: Active
---
# AI VM Architecture

## Overview

A dedicated Debian VM with an NVIDIA Quadro P5000 passed through, running LlamaFarm natively as the AI management layer. The VM is fully disposable — Ansible provisions the OS and installs tooling; models and data live on persistent storage. If the VM is destroyed, a fresh provision + persistent volume mount restores full capability in minutes.

## VM Specifications

| Component | Minimum | Recommended | Notes |
|-----------|---------|-------------|-------|
| vCPU | 4 cores | 8+ cores | Inference benefits from more cores for prompt processing |
| RAM | 16 GB | 32 GB | ~16 GB for OS + LlamaFarm services + 8B model inference + KV cache |
| Root disk | 40 GB | 64 GB | OS + LlamaFarm service binaries (~3-5 GB) + swap space |
| Model/data disk | 100 GB | 256 GB | Persistent volume for model weights + vector store + RAG data |
| GPU | Quadro P5000 | Quadro P5000 | 16 GB GDDR5X VRAM — comfortable for 7B-13B quantized models |

### Quadro P5000 Profile

| Spec | Value |
|------|-------|
| VRAM | 16 GB GDDR5X |
| CUDA cores | 2560 (Pascal architecture — compute capability 6.1) |
| CUDA toolkit | 11.x or 12.x (Pascal supports both; use 12.x for upstream llama.cpp compatibility) |
| PCIe | Gen3 x16 — ensure host has an available slot and IOMMU group isolation |
| TDP | 180 W — account for cooling in the chassis |

**Important**: Pascal (6.1) does not support `bfloat16` natively. LlamaFarm's Universal Runtime will use `fp16` or `fp32` fallback. Use GGUF quantized models (Q4_K_M) for best VRAM fit.

## Storage Layout

```
Filesystem                  Contents                    Size         Persistent?
/dev/sda1 (root)            OS + lf CLI + PyApp bins    40-64 GB     No — recreated
/dev/sdb1 or NFS mount      Models + data               100-256 GB   Yes — survives
```

**Persistent volume contents (`/var/lib/llamafarm` or mount at `~/.llamafarm`):**

```
~/.llamafarm/               # LF_DATA_DIR
├── bin/                    # PyApp service binaries (server, rag, runtime)
├── projects/               # Project configs, Chroma vector store, session state
└── addons/                 # ML addon wheels

~/.cache/huggingface/       # TRANSFORMERS_CACHE
└── hub/                    # Downloaded model weights (the real space hog)
```

**Size estimate with a typical setup:**

| Item | Size |
|------|------|
| PyApp service binaries | ~1-2 GB |
| Qwen3-8B (Q4_K_M GGUF) | ~5 GB |
| Qwen3-1.7B (Q4_K_M GGUF) | ~1 GB |
| all-MiniLM-L6-v2 (embeddings) | ~80 MB |
| Chroma vector store (RAG) | grows with ingested data (MBs to GBs) |
| **Total** | **~8-12 GB** excluding RAG data |

## Software Stack

```
┌─────────────────────────────────────────────────────┐
│                    User / Client                      │
│  (Tailscale SSH, chat UI at :14345, REST API)        │
│           Direct HTTP — no reverse proxy             │
├─────────────────────────────────────────────────────┤
│              LlamaFarm Server (:14345)                │
│           OpenAI-compatible REST API                  │
│           Designer Web UI                             │
├─────────────────────────────────────────────────────┤
│                                                     │
│   ┌──────────────┐   ┌──────────────┐              │
│   │ RAG Worker   │   │ Universal    │              │
│   │ (Celery)     │   │ Runtime      │              │
│   │              │   │ (:11540)     │              │
│   │ Parsing      │   │ HF Models    │              │
│   │ Chunking     │   │ GGUF via     │              │
│   │ Embedding    │   │ llama.cpp   │              │
│   │ ChromaStore  │   │ OCR/Classify │              │
│   └──────────────┘   └──────┬───────┘              │
│                              │                       │
│                    ┌─────────▼─────────┐            │
│                    │   NVIDIA CUDA      │            │
│                    │   Quadro P5000    │            │
│                    │   16 GB VRAM      │            │
│                    └───────────────────┘            │
├─────────────────────────────────────────────────────┤
|              Debian (latest stable)                   |
├─────────────────────────────────────────────────────┤
|              Hypervisor (Proxmox/ESXi)               |
└─────────────────────────────────────────────────────┘
```

## Provisioning Flow

### Day 0: Hypervisor

1. Pass through Quadro P5000 to VM via IOMMU group isolation
2. Allocate vCPUs, RAM, root disk
3. Attach persistent data volume (NFS or secondary vDisk)
4. Boot Debian netinstall

### Day 0: Ansible

```
ansible-playbook -i inventory/ai-vm playbooks/ai-vm.yml
```

The playbook:

1. **OS hardening**: fail2ban, unattended-upgrades, firewall (ports 14345, 11540 restricted to Tailscale IPs)
2. **NVIDIA driver + CUDA toolkit**: pin to version compatible with PyTorch in LlamaFarm
3. **Persistent volume mount**: format if needed, mount at `/var/lib/llamafarm`, symlink `~/.llamafarm` and `~/.cache/huggingface`
4. **Install LlamaFarm CLI**: `curl -fsSL ... | bash`
5. **Create systemd unit** for LlamaFarm services
6. **Verify**: `lf version`, `nvidia-smi`, CUDA test; access Designer UI at `http://ai-vm:14345`

### Day 1: LlamaFarm

```bash
lf init my-ai-vm
lf start                        # starts server + RAG worker + Universal Runtime
lf chat "Hello"                 # verify model works (pulls on first request)
lf datasets create ...          # ingest documents for RAG
```

## Model Configuration

```yaml
# ~/.llamafarm/projects/default/my-ai-vm/llamafarm.yaml
version: v1
name: my-ai-vm
namespace: default

runtime:
  default_model: balanced

  models:
    fast:
      provider: universal
      model: Qwen/Qwen2.5-1.5B-Instruct
      base_url: http://127.0.0.1:11540/v1

    balanced:
      provider: universal
      model: unsloth/Qwen3-8B-GGUF:Q4_K_M
      base_url: http://127.0.0.1:11540/v1
      extra_body:
        n_ctx: 4096
        n_gpu_layers: -1
        flash_attn: true

    embedder:
      provider: universal
      model: sentence-transformers/all-MiniLM-L6-v2
      base_url: http://127.0.0.1:11540/v1
      model_type: embedding

rag:
  databases:
    - name: main_db
      type: ChromaStore
      default_embedding_strategy: default_embeddings
      default_retrieval_strategy: semantic_search
      embedding_strategies:
        - name: default_embeddings
          type: UniversalEmbedder
          config:
            model: sentence-transformers/all-MiniLM-L6-v2
            base_url: http://127.0.0.1:11540/v1
      retrieval_strategies:
        - name: semantic_search
          type: BasicSimilarityStrategy
          config:
            top_k: 5
```

## Disposability Strategy

The VM can be destroyed and recreated with zero data loss:

| What | Lives where | Survives VM destroy? |
|------|-------------|---------------------|
| OS + `lf` binary | Root disk | No — `curl | bash` reinstalls |
| Model weights | `~/.cache/huggingface/` on persistent volume | **Yes** |
| Vector DB, project config, session | `~/.llamafarm/` on persistent volume | **Yes** |
| Ansible inventory + playbooks | Ansible control node (workstation or homelab management box) | **Yes** |
| `llamafarm.yaml` | Persistent volume + version-controlled in Ansible repo | **Yes** |

**Recovery sequence:**

1. Create VM, attach persistent volume
2. `ansible-playbook playbooks/ai-vm.yml`
3. `lf init my-ai-vm && lf start`
4. `lf chat` — works immediately, models already cached

Total time: ~5-15 minutes (mostly apt updates + NVIDIA driver compile/dkms).

## Service Management

Service managed via systemd:

```ini
[Unit]
Description=LlamaFarm AI Platform
After=network.target

[Service]
Type=simple
User=ai
Group=ai
Environment=LF_DATA_DIR=/var/lib/llamafarm
Environment=TRANSFORMERS_CACHE=/var/lib/llamafarm/cache
WorkingDirectory=/var/lib/llamafarm/projects/default/my-ai-vm
ExecStart=/usr/local/bin/lf start --daemon
Restart=on-failure
RestartSec=10

[Install]
WantedBy=multi-user.target
```

Ansible manages this unit file and can enable/start it.

## Network Topology

```
Client ─── Tailscale ─── AI VM (:14345)
(laptop)    (overlay)     ├─ LlamaFarm Server / Designer UI
                          ├─ Universal Runtime (:11540)
                          └─ RAG worker (internal)
```

- LlamaFarm API and Designer UI accessed directly over Tailscale at `http://ai-vm:14345`
- No reverse proxy needed — LlamaFarm serves HTTP natively
- Ports 14345 and 11540 bind to the Tailscale interface only
- No ports exposed to LAN or WAN directly

## Backup Strategy

| What | Frequency | Method |
|------|-----------|--------|
| `~/.cache/huggingface/` (models) | Once (re-downloadable) | Backup only if slow link. Models are cachable artifacts. |
| `~/.llamafarm/projects/` (configs + vector DB) | Daily | rclone to NAS (or any S3-compatible target) |
| Ansible playbooks | On change | Git — push to private forge |

## Current Limitations & Future Considerations

- **Pascal architecture** (compute 6.1) means no bfloat16. All inference is fp16 or fp32. Not a blocker, but newer GPUs (Turing/Ampere+) would be ~2x faster for the same model.
- **Single GPU** — can serve one model at a time efficiently. For multi-model serving concurrently, split VRAM across instances or add a second GPU.
- **LlamaFarm pre-1.0** — expect occasional breaking changes. Pin the `lf` version and test upgrades in a disposable LXC first.
- **Monitoring** — not yet addressed. Consider Prometheus + node_exporter or Uptime Kuma for health checks.
