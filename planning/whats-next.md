# What's Next

Tracking active tasks being carried out and their associated commits.

> **Purpose**: This file is the operational task list—what's actively being worked on right now. Each task should reference the commit hash when changes are made, providing a clear audit trail from task to code. For higher-level tracking, see `/BACKLOG.md`.

---

## Current Tasks

| Task | Status | Commit |
|------|--------|--------|
| — | — | *No active tasks* |

---

## Active Workstreams

*Create subdirectories under `planning/` for each project workstream.*

| Workstream | Directory | Status |
|------------|-----------|--------|
| — | — | *No active workstreams* |

---

## Recently Completed

- Reorganized planning/tracking file locations
- Added purpose documentation to BACKLOG, ACTIVITY, whats-next
- Defined co-creation behaviors (Storm Session, Pre-Mortem, Smooth Brain, Progressive Bookkeeping)
- Created `wip/` directory and `workstyle/working_style.md`
- Created `.cursor/rules/co-creation.mdc`
- Added Dead Drop behavior, Close-out compound behavior, and Conventions (Isolation, Append-Don't-Replace)
- Added Troubleshoot behavior for structured debugging and knowledge capture

---

# Dead Drop — 2026-04-22

**In progress:** Storm Session on workspace behaviors (`wip/workspace-behaviors/`) — session paused, not graduated

**Just completed:**
- Added Dead Drop behavior with Bread Crumb alias (944080e)
- Added Close-out compound behavior under Dead Drop (6a0873b)
- Added Conventions section: Isolation, Append-Don't-Replace (6a0873b)
- Updated Cursor rule to reflect all behaviors and conventions

**Next step:** Decide whether to graduate/discard the Storm Session, or continue adding behaviors. Roadmap Phase 3 mentions behavioral profiles (Architect, DevOps, etc.) as a future direction.

**Key decision:** Close-out is a compound behavior (Smooth Brain → Dead Drop), not a separate table entry. Conventions are always-on rules, distinct from invocable behaviors.

**Git state:** 6a0873b — Add Dead Drop behavior and conventions

**Open threads:** Storm Session `wip/workspace-behaviors/` still exists with scratch files; can be resumed or discarded

---

# Dead Drop — 2026-04-23

**In progress:** Storm Session on workspace behaviors (`wip/workspace-behaviors/`) — user had another idea but couldn't recall it

**Just completed:**
- Added Troubleshoot behavior to `workstyle/working_style.md`
- Updated `.cursor/rules/co-creation.mdc` with Troubleshoot
- Troubleshoot includes: investigation sessions, remediation docs, prevention docs, quick doc path
- Added "tags/categories for findability" to BACKLOG Ideas/Future

**Next step:** User had another idea to discuss — resume when remembered. Otherwise, Storm Session could be graduated/discarded since Troubleshoot behavior is complete.

**Key decision:** Troubleshoot is a behavior (not convention) because it has distinct trigger, session structure, and graduation flow that would encroach on existing behaviors if treated as a convention.

**Git state:** uncommitted changes — Troubleshoot behavior added

**Open threads:** 
- Storm Session `wip/workspace-behaviors/` still active
- Auto-load working_style for CLI agents — added to BACKLOG Ideas/Future

# Dead Drop — 2026-04-27

**In progress:** Dotfiles restructure — renaming `dotfiles/dotfiles` → `configs/`, flattening `config/` subdirectory.

**Just completed:**
- Troubleshot OpenShift CLI login timeout (proxy issue) — remediation doc created
- Migrated neovim treesitter.lua to new nvim-treesitter API (main branch)
- Added `<leader>gd` keymap for Snacks dashboard

**Next step:** Execute restructure plan — git mv, update ansible playbooks, update symlinks role, fix symlinks.

**Key decision:** Removing broken lazygit symlink, not creating lazygit config for now.

**Git state:** uncommitted changes in dotfiles repo

**Open threads:** None

---

# Dead Drop — 2026-04-24

**In progress:** Neovim config review session complete.

**Just completed:**
- Removed plugins: `comment.lua`, `mini-files.lua`, `csvview.lua`, `rainbow_csv.lua`, `notify.lua`
- Re-added `cameron-wags/rainbow_csv.nvim` for CSV (treesitter CSV highlighting was insufficient)
- Disabled treesitter highlighting for CSV files
- Consolidated plugins: mini-* → `mini.lua`, git-* → `git.lua`
- Renamed `init.lua` → `misc.lua` in plugins/
- Added descriptions to all 24 plugin files (with cursor-agent attribution)
- Deleted cruft: `lazyvim.json`, `map.txt`, `IDEAS.md`, `PLUGIN_GUIDE.md`
- Customized lualine: powerline separators, branch styling, removed encoding/fileformat/progress
- Added `<S-Tab>` keybinding for file info popup in nvim-tree
- Fixed trouble.lua modes config (was outside opts)
- Added `diagnostics_split` mode to trouble
- Created `AI_DISCLAIMER.md` at repo root

**Next step:** Commit the remaining changes (csv.lua, treesitter.lua updates).

**Key decision:** CSV highlighting handled by rainbow_csv.nvim with treesitter disabled for CSV filetype; noice uses built-in mini notifications (nvim-notify removed).

**Git state:** a24dc95 — uncommitted: csv.lua (new), treesitter.lua (csv highlight disabled), lazy-lock.json

**Open threads:** None — session complete.

---

# Dead Drop — 2026-05-04

**In progress:** Homelab greenfield rebuild planning — **GRADUATED** to `docs/homelab/`

**Just completed:**
- Storm Session: Planned homelab architecture with 5 environments (Prod, Test, Dev, DevNode, DevOCP)
- Decided on Komodo for GitOps (over K3s + ArgoCD) — less complexity, Docker-native
- Tooling stack: Terraform + Ansible for infra, Komodo for containers
- Three managed environments (Dev → Test → Prod) with promotion via TOML file moves
- Two sandbox environments (DevNode, DevOCP) intentionally unmanaged
- Created architecture diagrams (environment map, GitOps flow)
- Graduated to documentation: `docs/homelab/` with README, architecture, environments, decisions, repo-structure, roadmap
- Updated `workstyle/working_style.md` with Documentation Structure convention

**Next step:** Begin implementation — set up infrastructure repo, provision Komodo Controller, install Periphery agents on first managed node.

**Key decisions:**
- Komodo over K8s — operational simplicity, familiar Docker concepts
- DevDocker is Komodo-managed (not sandbox) — enables promotion path
- Poll-based GitOps (not webhooks) — simpler, no public endpoint needed
- Dedicated NUC for Komodo Controller — separate management plane

**Git state:** uncommitted — new docs in `docs/homelab/`, updated working_style.md

**Open threads:**
- Network topology undecided
- Storage strategy undecided (Unraid vs Synology roles)
- Specific workloads not fully enumerated
- Storm session `wip/homelab-rebuild/` can be deleted (Plex examples remain as reference)

---

> 2026-05-04 23:56 — Refined homelab repo structure: renamed `komodo-configs/` → `komodo/`, removed `_stacks/` (compose files inline with app), apps use subdirectories when they have multiple files (compose, Dockerfile), flat TOML when simple. Also: made `wip/` and `planning/` visible by default, added Sneaky behavior to hide under `.workspace/`. Overhauled READMEs for readability. Next: network topology or storage strategy decisions.

> 2026-05-05 00:11 — Converted architecture.md diagrams from Mermaid to ASCII art. Created CI/CD & GitOps blog brainstorm (`docs/blog/cicd-gitops-homelab-brainstorm.md`) covering declarative vs imperative, pull vs push, K8s vs Docker tradeoffs, Komodo decision logic. Created `docs/homelab/tailscale.md` with full Tailscale strategy: remote access, partner adversarial access model, Docktail for container exposure, installation by node type (direct vs operator vs Docktail). Next: network topology, storage strategy, or continue Tailscale ACL details.


# Dead Drop — 2026-05-05
**In progress:** Drafting Komodo TOML templates for Docker services (Komga, Uptime Kuma) and Docker stacks configuration.
**Just completed:**
- Drafted `docker-komga.toml` (media server service)
- Drafted `docker-uptimekuma.toml` (monitoring service)
- Drafted `docker-stacks-config.toml` (centralized Docker Compose definitions)
**Next step:** Validate TOML syntax and graduate drafts to `artifacts/komodo/docker/`.
**Key decision:** Using `stack.reference` to point to centralized Docker Compose definitions instead of inlining configs.
**Git state:** uncommitted changes — `wip/komodo-artifact-templates/` drafts
**Open threads:**
- Confirm naming convention for `artifacts/komodo/` subdirectories (proposed: `docker/`, `cicd/`, `services/`)
- Draft `README.md` for `artifacts/komodo/`

---

# Dead Drop — 2026-05-11

**In progress:** IoT planning for new house — **GRADUATED** to `docs/homelab/iot/`

**Just completed:**
- Storm Session: Full IoT architecture planning for new house
- Decided Zigbee (Inovelli Blue switches) over Z-Wave — ecosystem breadth, cheaper accessories
- Deferred door locks (detection over prevention philosophy) and garage door integration (ratgdo)
- Documented Home Assistant deployment strategy — Phase 1 (RPi bootstrap), Phase 2 (Prod Docker migration)
- Added Phase 3 stretch goal: automated failover to Test with Ansible/firewall automation
- Created decision records: smart-switch-protocol, door-locks, home-assistant-deployment (ADR)
- Documented HA workflows (door alerts, scenes, presence automation)
- Shopping list finalized (~$270-275 for Phase 1)

**Next step:** Order hardware (Zigbee coordinator, Inovelli switches, door sensors). When Unifi equipment arrives, configure VLANs and firewall rules for HA → IoT access.

**Key decisions:**
- Zigbee over Z-Wave — accessory ecosystem wins at scale
- HA on Prod VLAN with firewall rules to IoT — proper network segmentation
- Zigbee2MQTT over ZHA — enables coordinator backup portability for failover
- USB passthrough to Docker container — privileged mode accepted

**Git state:** db27540 — Add Home Assistant ADR

**Open threads:**
- Unifi equipment on order — network setup blocked until arrival
- Phase 3 failover requires second Zigbee coordinator (~$25) — purchase when ready to implement

---

> 2026-05-12 23:09 — Homelab: ADR for Komodo ResourceSync per-environment branch steering is documented under `docs/homelab/planning/adrs/` and linked from platform + planning docs; IoT ADRs stay in `docs/homelab/iot/decisions/` (no move). Commit for that work is **not** pushed yet — user has a suggested message (`docs(homelab): add Komodo ResourceSync branch ADR and link from planning docs` + optional body from chat). Pick up with `git status` / commit / push when ready.

---

# Dead Drop — 2026-05-13

**In progress:** OpenShift **vGPU + MTV** combined program (`wip/troubleshoot-openshift-vgpu-passthrough/investigation.md`) — host prep, VDDK/Quay tagging, ClusterPolicy; resume with next cluster change or teammate sync.

**Just completed:**
- **Homelab ADR:** Renamed Komodo ResourceSync ADR to `docs/homelab/planning/adrs/0001-komodo-resourcesync-branch-per-environment.md`; updated links across homelab docs, komodo scratch, AI ADR references (`faa02aa`, `ce19cd3`).
- **AI remote stack (`wip/ai-remote-webview/`):** Collapsed planning to `README.md` + `handbook.md` + `evolution.md`; added **Proposed** ADR `adr/0001-self-hosted-ai-bookkeeping-stack.md` (risks + mitigations); `scratch.md` points at the split layout; committed as `ecf9168`.
- **Same push series (recent):** OpenShift GPU/MTV guides + investigation log (`777263f`); graduated Ansible readiness kit to `artifacts/openshift/readiness-validation-ansible/`; `latest`-tag pitfalls doc; integrated-registry + vGPU Manager guides under `docs/guides/openshift/`.

**Next step:** OpenShift—continue ClusterPolicy / registry image work against NVIDIA doc. AI stack—spike AIonUI WebUI on a target host, or set ADR `0001` to **Accepted** when direction is firm.

**Key decision:** ADR numbering is **per stream** (`planning/adrs/0001-…` vs `wip/ai-remote-webview/adr/0001-…`), not one global repo sequence.

**Git state:** `ecf9168` — Add Self-hosted AI bookkeeping stack concepts (clean working tree at drop time)

**Open threads:** MTV VDDK migration off `latest` to `9.0.0.0` before deleting `latest`; NeMo Guardrails stays optional until base stack is boring; IoT still blocked on Unifi delivery per 2026-05-11 drop.
