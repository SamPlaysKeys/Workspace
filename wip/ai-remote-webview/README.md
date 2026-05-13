# AI remote WebView / self-hosted coding stack

Planning for a **next iteration** of this Workspace: a self-hosted stack where a **bookkeeping Git repo** stays mounted on a host you control, with **browser UIs** for coding (AIonUI WebUI) and chat/RAG (Open WebUI), backed by **one OpenAI-compatible LLM**.

| Read this | When |
|-----------|------|
| **This README** | Orientation in ~2 minutes |
| **[handbook.md](handbook.md)** | Host paths, Compose, Ansible, UI rules, security—all depth in one place (use in-editor outline / `#` anchors) |
| **[evolution.md](evolution.md)** | Build order, graduation from `wip/`, open decisions |

**Original one-file brainstorm:** [scratch-original.md](scratch-original.md) · **Scratch pointer:** [scratch.md](scratch.md) · **ADRs:** [adr/README.md](adr/README.md)

---

## Goals (short)

- **Single repo truth** — same idea as today: `docs/`, `wip/`, `workstyle/`, etc., on disk for humans and agents.
- **AIonUI** — primary coding / file / Git surface (WebUI server mode).
- **Open WebUI** — general chat + RAG on docs.
- **Docker Compose + optional Ansible** — reproducible host.

---

## Architecture

```mermaid
flowchart LR
  subgraph host[Proxmox / Linux Host]
    subgraph docker[Docker Engine]
      llm[LLM Backend\n(Ollama or vLLM)]
      owui[Open WebUI]
      aion[AionUi WebUI\n(Express server mode)]
      nemo[NeMo Guardrails\n(optional)]
      proxy[Reverse Proxy\n(Tailscale / Caddy)]
    end
    gitrepo[(Bookkeeping Git repo)]
  end
  proxy --> owui
  proxy --> aion
  owui --> llm
  aion --> llm
  nemo --> llm
  aion --- gitrepo
  owui --- gitrepo
```

**Flow:** you hit the proxy over HTTPS → each UI talks to the LLM (or an optional NeMo guardrailed endpoint) → both see the **same bind-mounted repo**.

---

## Components at a glance

| Piece | Role |
|-------|------|
| **LLM** | Ollama or vLLM; OpenAI-compatible API |
| **Open WebUI** | Chat, RAG, light file/terminal use |
| **AIonUI** | Coding cockpit, workspace + rules |
| **NeMo Guardrails** | Optional policy layer in front of the LLM |
| **Proxy** | TLS + names (or Tailscale Serve instead) |

Details: [handbook.md § Components](handbook.md#components).

---

## Where this lives in the repo

`wip/ai-remote-webview/` is **exploratory**. When something ships, it can move to `artifacts/`, `docs/guides/`, or a separate infra repo—see [evolution.md](evolution.md).
