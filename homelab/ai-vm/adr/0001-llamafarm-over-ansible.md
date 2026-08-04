---
layout: page
title: 'ADR 0001: LlamaFarm as AI model management layer'
category: Homelab
status: Active
---
# ADR 0001: LlamaFarm as AI model management layer

**Status:** Proposed  
**Date:** 2026-07-14

## Context

The homelab has a dedicated AI VM running Ubuntu with assorted AI tools installed ad-hoc. There is no consistent model management layer — models are pulled manually, runtimes are configured individually (Ollama, vLLM, raw HuggingFace), and there is no unified API surface or RAG pipeline.

As part of a broader effort to make this VM "more than just a box with AI tools on it," a management layer is needed for:

- Model lifecycle (download, serve, switch, retire)
- RAG pipeline (ingest documents, chunk, embed, query)
- Unified API surface (OpenAI-compatible endpoint for all models)
- Reproducible configuration (infra-as-code for AI, not just the VM)

## Decision Drivers

1. **Disposable VM model** — the VM should be recreatable from Ansible + persistent data volumes without manual recovery steps.
2. **Local-first, offline-capable** — the AI layer must work without internet after models are cached.
3. **GPU passthrough** — quad-port P5000 needs CUDA support with minimal friction.
4. **Config-driven, not script-driven** — model selection, prompt templates, and RAG pipelines should live in version-controlled config files, not shell scripts.
5. **OpenAI-compatible API** — any tool that speaks OpenAI's API should be able to target this VM.

## Options Considered

### Option 1: Ansible/Terraform/Semaphore only

Provision the VM, install Ollama/vLLM directly, manage models via shell scripts or ad-hoc commands.

- **Pro:** Mature tooling, full homelab coverage.
- **Con:** No model lifecycle abstraction — still need glue scripts for model switching, RAG pipelines, etc. Ansible does not natively understand "serve this model" as a primitive.

### Option 2: LlamaFarm as the AI layer (chosen)

Provision the VM with Ansible, then run LlamaFarm natively (no containers) to manage models, RAG, and API serving.

- **Pro:** Config-driven (`llamafarm.yaml`), unified API, multi-model switching, built-in RAG, MCP support, no-container-required.
- **Con:** Pre-1.0 project (v0.0.34, YC W22), active development, small community.

### Option 3: Kubernetes + Kubeflow / KServe

Run a lightweight K8s cluster on the VM, deploy models via KServe or similar.

- **Pro:** Production-grade scaling, model canarying.
- **Con:** Massive operational overhead for a single-VM homelab. K8s on a single node adds complexity with zero benefit at this scale.

### Option 4: Docker Compose stack (Ollama + Open WebUI + Qdrant + custom API wrapper)

Orchestrate individual OSS components via Compose.

- **Pro:** Each component is mature individually.
- **Con:** Glue code for integration, more surface area to maintain, no single config file.

## Decision

**Adopt LlamaFarm** as the AI model management layer, running natively (no containers) on a Debian VM with Ansible for infrastructure provisioning.

Ansible handles:
- OS hardening, NVIDIA driver + CUDA toolkit installation
- Persistent storage mount points
- LlamaFarm CLI installation (`lf`) via `curl | bash`
- `$LF_DATA_DIR` and `$TRANSFORMERS_CACHE` redirection to persistent volumes
- systemd unit for LlamaFarm services
- Reverse proxy (nginx) with TLS termination

LlamaFarm handles:
- Model downloading, caching, and serving
- Multi-model configuration (fast vs powerful, local vs remote)
- RAG pipeline (document parsing, embedding, retrieval)
- OpenAI-compatible REST API
- MCP tool integration

## Consequences

### Positive

- **Single config file** (`llamafarm.yaml`) is the source of truth for all AI behavior — model selection, prompts, RAG, MCP servers.
- **Truly disposable VM** — models and data live on persistent volumes; a fresh Debian install + one `lf init` + `lf start` + pointing at cached models restores full capability.
- **Built-in RAG** — no need to wire up a separate vector DB, embedder, and chunker; it's all in the config.
- **Native deployment** — LlamaFarm runs without Docker/K8s, keeping the footprint lean and the GPU path simple.
- **Offline-capable** — after initial model download, all inference is local with no egress.

### Negative / Risks

| Risk | Impact |
|------|--------|
| **Pre-1.0 stability** | API/config changes between releases; may need to pin versions. |
| **Small community** | 833 GitHub stars, pre-1.0. Less troubleshooting help available compared to Ollama or Ansible. |
| **PyApp binary size** | Service binaries bundle Python runtimes (~1-2 GB total for server + rag + runtime). |
| **YC-backed startup** | Open-source today, but direction could shift. Fork as mitigation. |
| **Dependency chain** | Python + TypeScript + Go + Nx monorepo. More moving parts than Ollama alone. |
| **CUDA compatibility** | Requires matching CUDA toolkit version with PyTorch / llama.cpp. Upstream already dropped CUDA 11 (b7694+). |

### Mitigations

| Mitigation | Detail |
|------------|--------|
| **Pin LlamaFarm version** | Use `lf bundle --version v0.0.x` to create offline install bundles; avoid `latest` drift. |
| **Persistent model cache** | `$TRANSFORMERS_CACHE` and `$LF_DATA_DIR` on NAS-backed NFS or dedicated disk — models survive VM recreation. |
| **Ansible as day-0 only** | Ansible provisions the VM and installs `lf`; LlamaFarm's yaml is the AI config. Clear boundary reduces coupling. |
| **Monitor upstream** | Keep an eye on breaking changes before applying `lf` upgrades. Test in a disposable LXC first if uncertain. |
| **NVIDIA driver pinning** | Pin CUDA toolkit version in Ansible to match LlamaFarm's PyTorch requirement. |

## References

- LlamaFarm docs: https://docs.llamafarm.dev
- GitHub: https://github.com/llama-farm/llamafarm (Apache 2.0, 833 stars, v0.0.34)
- AI VM architecture: `docs/homelab/ai-vm/architecture.md`
