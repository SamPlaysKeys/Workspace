# Self‑Hosted AI Coding & Bookkeeping Stack (Docker / Ansible Plan)

## 1. Goals

- Run a **collaborative coding and troubleshooting environment** around a single “bookkeeping” Git repo.
- Provide:
  - **AIonUI WebUI** as the primary coding/collab surface with file manager and Git visibility.
  - **Open WebUI** as a general chat + RAG interface.
  - **LLM backend** (local or remote) shared by both.
- Control deployment with **Docker** (Docker Compose) and/or **Ansible** as IaC.

---

## 2. High‑Level Architecture

```mermaid
flowchart LR
  subgraph host[Proxmox / Linux Host]
    subgraph docker[Docker Engine]
      llm[LLM Backend\n(Ollama or vLLM)]
      owui[Open WebUI]
      aion[AionUi WebUI\n(Express server mode)]
      nemo[NeMo Guardrails\n(optional service)]
      proxy[Reverse Proxy\n(Tailscale Serve / Caddy / Nginx)]
    end

    gitrepo[(Bookkeeping Repo\n/docs, /wip, /troubleshooting, /activity)]
    vault[(Secrets / Vault\n(optional))]
  end

  proxy -->|HTTPS| owui
  proxy -->|HTTPS| aion

  owui --> llm
  aion --> llm
  nemo --> llm

  aion --- gitrepo
  owui --- gitrepo
```

---

## 3. Components

### 3.1 LLM Backend

**Options**

- **Ollama container** (simple, good local models).
- **vLLM container** backed by GPU for higher throughput.

Both should expose an **OpenAI‑compatible HTTP API** (e.g., `/v1/chat/completions`) for Open WebUI and AIonUI to use.[web:54][web:57]

**Key points**

- Mount models/cache to a host directory for persistence:
  - `/srv/llm/models:/models`
  - `/srv/llm/cache:/cache`
- Configure API port (e.g. `11434` for Ollama, `8000` for vLLM).

---

### 3.2 Open WebUI

- Deployed via **Docker**, the officially recommended path.[web:54][web:57]
- Can run as a single container with a persistent data volume:
  - `open-webui-data:/app/backend/data`[web:57]
- Connects to your LLM backend via HTTP (Ollama, vLLM, or other OpenAI‑compatible providers).[web:54][web:57]

**Planned usage**

- General chat, documentation Q&A, and RAG on the bookkeeping repo (docs).
- Optional Open Terminal + file browser to inspect/edit files in the repo from the browser.
- Optional: mount the bookkeeping repo into a terminal workspace for ad‑hoc editing.

---

### 3.3 AIonUI (WebUI mode)

AIonUI is normally a desktop app but also runs a **headless WebUI server mode**, exposing an Express HTTP + WebSocket API on a configurable port for browser access.[web:16][web:56]

- Upstream provides a **Dockerfile** you can use to build a container image.[web:63]
- In WebUI mode:
  - Use `AionUi --webui --remote` (or equivalent entrypoint) to allow LAN/remote connections.[web:16]
  - Data persists in a local SQLite DB inside the container or a mounted volume.[web:56]

**Planned usage**

- Primary **coding/troubleshooting cockpit**:
  - Attach the bookkeeping repo directory as a workspace.
  - Use file manager + previews for `/docs`, `/wip`, `/troubleshooting`, `/activity`, etc.
  - Configure assistants with Rules tailored to this repo and its conventions.[web:41][web:56]
- Connect AIonUI to the same LLM backend as Open WebUI.

---

### 3.4 Optional: NeMo Guardrails Service

- A small **Python service** (FastAPI/Flask) wrapping the LLM backend with **NeMo Guardrails** for policy/behavior control.[web:14]
- Exposes an OpenAI‑compatible endpoint:
  - Open WebUI and AIonUI can point at **either**:
    - The raw LLM backend, or
    - The guardrailed endpoint.

**Planned usage**

- Enforce:
  - File operation restrictions.
  - Response formats (e.g. “always propose a patch, don’t apply without confirmation”).
  - Domain‑specific rules for the bookkeeping repo.

---

### 3.5 Reverse Proxy / Network

- Either:
  - **Tailscale Serve** directly to containers (simple, fits your existing pattern), or
  - **Caddy / Nginx container** as a traditional reverse proxy.

**Responsibilities**

- TLS termination.
- Virtual hosts:
  - `ai.lab.local` → AIonUI WebUI
  - `chat.lab.local` → Open WebUI
- Optional basic auth / JWT / IP allow‑listing.

---

## 4. Directory & Repo Layout

On the host:

```text
/srv/
  llm/
    models/
    cache/
  ai/
    open-webui/
      data/
    aionui/
      data/
  workspaces/
    bookkeeping/
      .git/
      docs/
      wip/
      troubleshooting/
      activity/
      configs/
```

- `workspaces/bookkeeping/` is the Git repo.
- `docs/`, `wip/`, `troubleshooting/`, `activity/` are the main collaboration zones for the AI + you.
- `configs/` can hold structured bookkeeping configs and rules.

---

## 5. Docker Compose Plan

### 5.1 Compose File Structure

```text
infra/
  docker-compose.yml
  .env
  ansible/
    site.yml
    roles/
      docker/
      openwebui/
      aionui/
      llm/
      proxy/
```

- `docker-compose.yml`: defines all services.
- `.env`: port mappings, image tags, secrets references.
- `ansible/`: playbooks to deploy/configure Docker and the stack (see section 6).

### 5.2 Services Overview

**Services:**

1. `llm-backend` (Ollama or vLLM)
2. `open-webui`
3. `aionui` (built from upstream Dockerfile)
4. `nemo-guardrails` (optional)
5. `reverse-proxy` (optional if using Tailscale Serve)

**Volumes:**

- `llm-models` → `/srv/llm/models`
- `llm-cache` → `/srv/llm/cache`
- `open-webui-data` → `/srv/ai/open-webui/data`
- `aionui-data` → `/srv/ai/aionui/data`
- `bookkeeping-workspace` (bind mount) → `/srv/workspaces/bookkeeping`

### 5.3 Skeleton Compose (conceptual)

> Note: Snippet is intentionally schematic; you’ll fill in exact images/tags.

```yaml
version: "3.9"

services:
  llm-backend:
    image: ollama/ollama:latest   # or custom vLLM image
    ports:
      - "11434:11434"
    volumes:
      - /srv/llm/models:/models
      - /srv/llm/cache:/cache
    restart: unless-stopped

  open-webui:
    image: ghcr.io/open-webui/open-webui:main  # official image[1][2]
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
      dockerfile: Dockerfile  # based on upstream Dockerfile[3]
    depends_on:
      - llm-backend
    environment:
      - LLM_API_BASE=http://llm-backend:11434
      - LLM_API_KEY=dummy
    command: ["AionUi", "--webui", "--remote"]  # server mode[4][5]
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
    image: caddy:latest  # or nginx
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

---

## 6. Ansible Plan

Your IaC can use **Ansible to manage Docker + Compose**, rather than choosing only one.

### 6.1 Roles

- `roles/docker/`
  - Install Docker Engine & docker‑compose plugin.
  - Configure user/group, daemon options.

- `roles/llm/`
  - Create `/srv/llm/models`, `/srv/llm/cache` with correct permissions.
  - Optionally pre‑pull large models or seed configs.

- `roles/openwebui/`
  - Create `/srv/ai/open-webui/data`.
  - Template `.env` and `docker-compose.yml` sections for `open-webui`.
  - Optionally manage a `config.yaml` for providers.

- `roles/aionui/`
  - Clone AIonUI repo or download build artifact.
  - Build container image from upstream Dockerfile.[web:63]
  - Ensure `/srv/ai/aionui/data` exists; manage env config.

- `roles/workspaces/`
  - Ensure `/srv/workspaces/bookkeeping` exists.
  - Clone your bookkeeping repo (Git) and set origin.
  - Manage directory structure: `docs/`, `wip/`, `troubleshooting/`, `activity/`, etc.

- `roles/proxy/`
  - Deploy Caddy/Nginx config or Tailscale Serve config.

- `roles/compose/`
  - Deploy the `docker-compose.yml` template to `/srv/infra/docker-compose.yml`.
  - Run `docker compose up -d` via `community.docker.docker_compose`.

### 6.2 Example Top‑Level Playbook (conceptual)

```yaml
- hosts: ai_hosts
  become: true
  vars:
    llm_backend: "ollama"
  roles:
    - docker
    - workspaces
    - llm
    - openwebui
    - aionui
    - nemo-guardrails
    - proxy
    - compose
```

---

## 7. Configuration / Behavior Rules

### 7.1 AIonUI Assistants

- Define assistants in AIonUI with Rules (per its Assistant Configuration Guide) to:
  - Only modify files inside `/workspace`.
  - Map tasks to specific directories:
    - Documentation → `/workspace/docs/`
    - WIP drafts → `/workspace/wip/`
    - Troubleshooting logs → `/workspace/troubleshooting/`
    - Activity summaries → `/workspace/activity/`
  - Require explicit confirmation for large refactors or destructive operations.[web:41][web:56]

### 7.2 Open WebUI Projects / Workspaces

- Use Open WebUI **Projects/Folders** to:
  - Create a “Bookkeeping” project.
  - Attach the RAG knowledge base to `/workspace/docs` and other relevant directories (via file uploads or shared volume paths).[web:54][web:57]
  - Configure prompts that remind the model of the bookkeeping structure and constraints.

---

## 8. Security & Access

- Run everything on a **non‑public host**.
- Access options:
  - **Tailscale SSH / Serve** to expose:
    - `https://ai.lab.tailnet/` → AIonUI WebUI
    - `https://chat.lab.tailnet/` → Open WebUI
  - Or Caddy/Nginx with TLS certs and IP/JWT restrictions.

- Store sensitive API keys (if you use remote models) in:
  - Ansible Vault.
  - HashiCorp Vault, with Ansible retrieving them at deploy time.

---

## 9. Next Steps

1. Decide on **LLM backend** (Ollama vs vLLM) and GPU strategy.
2. Finalize the **host paths** in `/srv` and your Git remote for the bookkeeping repo.
3. Author:
   - `infra/docker-compose.yml` based on the skeleton.
   - Ansible roles as outlined, using `community.docker` collection.
4. Iterate on:
   - AIonUI assistant Rules tailored to your bookkeeping conventions.
   - Open WebUI prompts & RAG configuration for the same repo.

This markdown should be enough to bootstrap your IaC design: you can translate each section directly into Ansible roles and Docker Compose services while keeping the architecture clean and reproducible.
