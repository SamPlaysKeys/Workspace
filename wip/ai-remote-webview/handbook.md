# Stack handbook

Single reference for **implementation detail**. Skim headings in your editor outline; jump with `#anchors` (e.g. `handbook.md#docker-compose`).

**Architecture decisions (trade-offs, risks):** [adr/0001-self-hosted-ai-bookkeeping-stack.md](adr/0001-self-hosted-ai-bookkeeping-stack.md) (Proposed ADR).

---

## Components

### LLM backend

- **Ollama** — simple container ops, good local models.
- **vLLM** — higher throughput with a **GPU**.

Expose an **OpenAI-compatible HTTP API** (e.g. `/v1/chat/completions`) so both UIs integrate the same way.

**Persistence:** mount host dirs for weights and cache, e.g. `/srv/llm/models`, `/srv/llm/cache`. Confirm container-internal paths in the image docs (they are not always `/models`).

---

### Open WebUI

- Deploy with **Docker** (upstream’s usual path).
- Persistent volume for app data, e.g. host `/srv/ai/open-webui/data` → container backend data dir.
- Point **OpenAI API base URL** at Ollama, vLLM, or another compatible endpoint.

**Usage:** general chat, doc Q&A, **RAG** on `docs/` (and other roots you choose). Optional terminal/file access against the same bind-mounted repo.

---

### AIonUI (WebUI / server mode)

Headless **Express + WebSocket** server for browser use; build from upstream **Dockerfile** where available. Persist DB/state under e.g. `/srv/ai/aionui/data`. Verify **CLI flags and ports** for your release before fixing the compose entrypoint.

**Usage:** primary **coding / troubleshooting** surface—repo as `/workspace`, rules aligned to `docs/`, `wip/`, etc. Same LLM base URL as Open WebUI unless you intentionally split models.

---

### Optional: NeMo Guardrails

Small service in front of the LLM with **NeMo Guardrails**; exposes an OpenAI-compatible endpoint. UIs can target **raw LLM** or **guardrailed** URL. Use when policy (file bounds, “propose don’t apply”) justifies the extra moving part.

---

### Reverse proxy

**Tailscale Serve** to container ports, or **Caddy / Nginx** for TLS and virtual hosts (e.g. `ai.*` → AIonUI, `chat.*` → Open WebUI). Add auth / allow-lists if needed.

---

## Host directory layout

Illustrative paths on the Linux host:

```text
/srv/
  llm/models/   llm/cache/
  ai/open-webui/data/   ai/aionui/data/
  workspaces/bookkeeping/   ← Git repo (.git, docs/, wip/, …)
```

**Rule:** one Git root at a **stable** mount (e.g. `/workspace`) shared by services that should see the same files.

Align folder names with this repo (`planning/` vs `activity/`, `docs/troubleshooting/` vs `troubleshooting/`) when you implement so assistant rules match reality.

---

## Docker Compose

**Infra repo sketch:**

```text
infra/docker-compose.yml
.env
ansible/site.yml
ansible/roles/{docker,llm,openwebui,aionui,workspaces,proxy,compose}
```

**Services (conceptual):** `llm-backend`, `open-webui`, `aionui`, optional `nemo-guardrails`, optional `proxy`.

**Volumes:** durable dirs for LLM data, each UI’s state, and a **bind mount** of the bookkeeping repo into the UI containers.

### Skeleton compose (schematic)

> Verify image env vars, internal paths, and ports against **current** upstream docs before use.

```yaml
version: "3.9"

services:
  llm-backend:
    image: ollama/ollama:latest
    ports:
      - "11434:11434"
    volumes:
      - /srv/llm/models:/models
      - /srv/llm/cache:/cache
    restart: unless-stopped

  open-webui:
    image: ghcr.io/open-webui/open-webui:main
    depends_on:
      - llm-backend
    environment:
      - OPENAI_API_BASE=http://llm-backend:11434
      - OPENAI_API_KEY=dummy
    ports:
      - "3000:8080"
    volumes:
      - /srv/ai/open-webui/data:/app/backend/data
      - /srv/workspaces/bookkeeping:/workspace
    restart: unless-stopped

  aionui:
    build:
      context: ./aionui
      dockerfile: Dockerfile
    depends_on:
      - llm-backend
    environment:
      - LLM_API_BASE=http://llm-backend:11434
      - LLM_API_KEY=dummy
    command: ["AionUi", "--webui", "--remote"]
    ports:
      - "3100:3000"
    volumes:
      - /srv/ai/aionui/data:/data
      - /srv/workspaces/bookkeeping:/workspace
    restart: unless-stopped

  nemo-guardrails:
    image: your-registry/nemo-guardrails:latest
    depends_on:
      - llm-backend
    environment:
      - LLM_API_BASE=http://llm-backend:11434
    ports:
      - "8200:8000"
    restart: unless-stopped

  proxy:
    image: caddy:latest
    ports:
      - "80:80"
      - "443:443"
    volumes:
      - ./proxy/Caddyfile:/etc/caddy/Caddyfile
    depends_on:
      - open-webui
      - aionui
    restart: unless-stopped
```

Pin **image tags or digests** for anything beyond a throwaway experiment ([`../../docs/troubleshooting/containers/container-image-latest-tag-pitfalls.md`](../../docs/troubleshooting/containers/container-image-latest-tag-pitfalls.md)).

---

## Ansible

Use **Ansible** to install Docker, create `/srv/...`, template `.env` / compose, and run **`community.docker`** compose modules.

| Role | Does |
|------|------|
| `docker/` | Engine + compose plugin |
| `llm/` | Dirs, permissions, optional model prefetch |
| `openwebui/` | Data dir, templates |
| `aionui/` | Clone/build image, data dir |
| `workspaces/` | Clone bookkeeping repo, scaffold dirs |
| `proxy/` | Caddyfile or Tailscale Serve doc |
| `compose/` | Render compose + `docker compose up -d` |

**Secrets:** Ansible Vault or HashiCorp Vault at deploy time—not committed `.env`.

---

## UI rules and RAG

**AIonUI:** assistants constrained to `/workspace`; map doc vs wip vs troubleshooting vs planning in rules; require confirmation for large/destructive edits. Prefer **rules in Git** (markdown) so they version with the repo.

**Open WebUI:** a “Bookkeeping” project; RAG roots biased to **`docs/`** unless you intend to index all of `wip/`. Prompts should mention `workstyle/working_style.md`, Dead Drop location, disposable `wip/`.

**Both UIs:** same **API base URL** (or same guardrailed URL); **one bind mount** of the repo so nothing drifts.

---

## Security

- Prefer a **non-public** host; edge via **Tailscale** and/or TLS proxy.
- Never commit real `.env`; ship `.env.example` only.
- Pin images; rebuild AIonUI from known commits on upgrades.
