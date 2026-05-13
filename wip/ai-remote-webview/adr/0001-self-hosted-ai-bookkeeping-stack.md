# ADR: Self-hosted AI stack for bookkeeping repo (Open WebUI + AIonUI + LLM)

**Status:** Proposed  
**Date:** 2026-05-14  

## Context

This workspace treats a **single Git repository** as the system of record for documentation, WIP, troubleshooting notes, and collaboration conventions (`docs/`, `wip/`, `workstyle/`, `planning/`, etc.). Today that work is driven mostly from a **local clone** plus desktop tools (IDE, Cursor).

A planned **next iteration** (see `wip/ai-remote-webview/README.md` and `handbook.md`) would run:

- **AIonUI** in **WebUI / server mode** as the primary **coding and repo-facing** surface.
- **Open WebUI** for **general chat and RAG** over documentation.
- A **shared LLM backend** with an **OpenAI-compatible HTTP API** (e.g. Ollama or vLLM in Docker).
- Optional **NeMo Guardrails** in front of the LLM for policy.
- **Docker Compose** for runtime, with **Ansible** optional for provisioning.
- **Private access** (e.g. Tailscale, reverse proxy with TLS).

The bookkeeping repo would be **bind-mounted** at a stable path (e.g. `/workspace`) so both UIs and humans see the same tree.

This ADR records the **directional decision** to pursue that stack, explicit **concerns**, and **mitigations**—so trade-offs are reviewable before significant time or hardware is committed.

## Decision

1. **Adopt (in principle)** a self-hosted, browser-first stack composed of **LLM + Open WebUI + AIonUI**, with **Compose** as the packaging boundary and **Ansible** as the optional day-0 / reprovisioning layer.

2. **Treat the bookkeeping Git repo as the single source of truth** for content and conventions; **do not** fork a second copy of the repo for routine agent work on the same host.

3. **Keep NeMo Guardrails optional** until the base path (LLM + one UI + mount + backup) is stable.

4. **Defer** “production-grade” multi-tenant hardening until scope is single-user or small trusted group on a non-internet-facing path.

## Consequences

### Positive

- **Clear separation of concerns:** coding / file / Git workflows vs chat + RAG, without forcing one UI to do everything.
- **One LLM integration pattern** (OpenAI-compatible API) for both surfaces.
- **Reproducibility:** Compose (and later Ansible) can version infra next to or alongside the repo.
- **Alignment with existing workspace habits:** same directories, same workstyle docs, portable to a always-on host.

### Negative / risks (including reviewer concerns)

| Risk | Why it matters |
|------|----------------|
| **Operational surface area** | Two UIs (plus LLM, plus optional guardrails and proxy) means **more images, upgrades, backups, and failure modes** than a single chat deployment. |
| **AIonUI WebUI mode maturity** | Web/server mode is **less commodity** than Open WebUI or a generic code-server; **entrypoints, ports, and persistence** can shift between upstream releases—higher integration risk. |
| **Git + multi-writer semantics** | Agents and humans (and optional Open WebUI file features) may **touch the same tree**; without discipline you get merge noise, accidental large edits, or partial commits. |
| **RAG scope and data hygiene** | Indexing beyond `docs/` (e.g. all of `wip/`) can **pull in noise, drafts, or secrets** if any credential ever lands in-repo. |
| **NeMo Guardrails cost** | Extra service to build, secure, and upgrade; **benefit must justify** ops time at small scale. |
| **Supply chain** | Many third-party images; without **pinned tags/digests** you inherit drift and unclear rollback (see workspace doc on `latest` tags). |
| **“Persistent session” confusion** | The stack gives a **durable host and repo**, not infinite LLM context; expectations must stay aligned or users will feel the system “forgets” across sessions. |

### Mitigations (recommended)

| Mitigation | Detail |
|------------|--------|
| **Phased rollout** | Prove **LLM + one UI + bind mount** first; add second UI; then proxy; only then optional NeMo (matches `evolution.md` build order). |
| **Pin and document versions** | Image digests or semver tags; record AIonUI **git ref** used for builds. |
| **Git discipline** | Rules: when agents may commit; prefer branches for large experiments; **backup** or snapshot repo before broad agent edits. |
| **RAG boundaries** | Start RAG on **`docs/`** only; widen only with explicit review. |
| **Assistant / project prompts** | Encode directory semantics and `workstyle/` references in both UIs (see `handbook.md` § UI rules and RAG). |
| **NeMo decision gate** | Add NeMo only if a written policy (file ops, “propose don’t apply”) cannot be enforced acceptably via UI rules alone. |
| **Private network first** | Tailscale or equivalent before any public ingress; secrets in Vault / Ansible Vault, not git. |

## Alternatives considered

| Alternative | Why not chosen (for this ADR) |
|-------------|------------------------------|
| **Open WebUI only** | Simpler ops, but a weaker **first-class coding / Git** story for the bookkeeping workflow you described. |
| **Generic code-server + chat** | Mature editing, but **does not** match the AIonUI-specific plan and assistant model you outlined; revisit if AIonUI WebUI proves unsuitable. |
| **SaaS only (Cursor Cloud, etc.)** | Offloads ops; conflicts with **self-hosted** and **repo-local** control goals for this iteration. |
| **Single combined “super UI”** | Theoretical; no single product was identified that cleanly replaces **both** roles without compromise—kept as future revisit. |

## References

- Planning hub: `wip/ai-remote-webview/README.md`
- Technical detail: `wip/ai-remote-webview/handbook.md`
- Roadmap / graduation: `wip/ai-remote-webview/evolution.md`
- Image tagging pitfalls: `docs/troubleshooting/containers/container-image-latest-tag-pitfalls.md`
- ADR format example: `docs/homelab/planning/adrs/0001-komodo-resourcesync-branch-per-environment.md`
