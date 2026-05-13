# Roadmap and graduation

## Build order

1. Choose **LLM** (Ollama vs vLLM) and GPU strategy.
2. Freeze **host paths** and Git remote for the bookkeeping repo.
3. Write **`infra/docker-compose.yml`** — prove **LLM + one UI** first, then add the second UI and proxy.
4. Add **Ansible** (Docker, dirs, templated compose, `docker compose up`) when the manual path works.
5. Tune **AIonUI rules** and **Open WebUI** RAG/prompts (see [handbook.md § UI rules and RAG](handbook.md#ui-rules-and-rag)).

---

## Graduation: Workspace → “persistent session”

**Persistent session** here means a **durable host + mounted repo + web UIs**, not one infinite model context. Same files and conventions as today; agents and humans meet on infrastructure you operate.

| Stage | Location | Meaning |
|-------|----------|---------|
| Now | This repo, `wip/ai-remote-webview/` | Planning only |
| Design locked | This repo or **`infra/`** repo | Compose + Ansible in git |
| Running | Proxmox VM (etc.) | Docker up; repo at `/srv/workspaces/bookkeeping` |
| Ops docs | `docs/guides/` or `artifacts/` | Backup, upgrades, key rotation |

**Split repos sensibly:** keep `workstyle/`, `.cursor/rules`, etc. in the **bookkeeping** tree; keep **secrets** and host-only overrides off-repo or in a **private** infra repo.

---

## Open decisions

- One repo vs split **bookkeeping** vs **infra**.
- Canonical names: `activity/` vs `planning/`, `troubleshooting/` vs `docs/troubleshooting/`.
- Whether **NeMo Guardrails** earns its ops cost at your scale.

When resolved, shorten this file to a checklist and bump [README.md](README.md) if the architecture blurb changes. **Formal trade-offs:** see [adr/0001-self-hosted-ai-bookkeeping-stack.md](adr/0001-self-hosted-ai-bookkeeping-stack.md) (Proposed).
