# Backlog

Human-in-the-loop review and tracking of what has been done and what is in progress.

> **Purpose**: This file is for high-level tracking of work items that benefit from human review and prioritization. It's the place to capture ideas, track progress on larger initiatives, and mark things complete. Unlike automated task tracking, this is intentionally manual to keep a human in the loop.

---

## In Progress

- [x] **AI VM Management**: LlamaFarm-based AI model management layer on Debian + Quadro P5000 — ADR 0001, architecture plan, and Ansible draft → graduated to `docs/homelab/ai-vm/`
- [ ] **Storm Session: Tailscale IDP for OpenBao**: Brainstorm and research auth methods for node identity integration (`wip/tailscale-idp-openbao/`)
- [ ] **Storm Session: OpenShift Virtualization VM Migration**: Develop tuning profiles, orchestration pathways, and decision trees for near-zero-downtime upgrades (`wip/openshift-virtualization-VM-migration/`)
- [x] **Storm Session: RHACM/Argo Drift Management System**: Design drift detection, classification, and remediation workflows → graduated to `docs/platform/argo-rhacm-drift-management/`

- [ ] **Homelab — public status site** (`status.samplayskeys.com`): VPS-hosted, user-facing component status; see `docs/homelab/observability/status-and-operator-dashboard.md`
- [ ] **Homelab — operator single pane:** lab overview + links (Uptime Kuma, Komodo, dynamic management URLs); see same document
- [x] **OpenShift vGPU + MTV combined program:** Host prep, VDDK pinning, and ClusterPolicy definition (Step 5 complete)
- [ ] Populate `artifacts/` with reusable automation components
- [ ] Create initial skills in `workstyle/skills/`
- [ ] Set up pre-commit hooks for secret scanning

---

## Completed

- [x] **HCL BigFix Setup & Planning Guide**: Created a comprehensive guide under `docs/guides/bigfix/planning.md` detailing architecture, sizing, prerequisites, configuration flow, and consulting discovery questions.
- [x] **AI VM Management**: ADR 0001 (LlamaFarm over Ansible), architecture plan, Ansible draft — graduated from `wip/ai-vm-management/` to `docs/homelab/ai-vm/`
- [x] **OpenShift vGPU + MTV combined program:** Host prep, VDDK pinning, and ClusterPolicy definition (Step 5 complete)
- [x] **Git History Refactor**: Change commit author from `vault@tolaria.md` to `sam@samplayskeys.com`
- [x] **VSCode Setup Guide for Enterprise Development**: Documentation for VSCode plugins and workflows (OpenShift, Ansible, Python, YAML, Bash, Git, PATs, and prerequisites).
- [x] Initial repository structure planning
- [x] Reorganize directory structure to match revised plan
- [x] Establish `planning/` directory for workstream management
- [x] Codify purpose of tracking files (BACKLOG, ACTIVITY, whats-next)
- [x] Define co-creation behaviors in `workstyle/working_style.md` (Storm Session, Pre-Mortem, Smooth Brain, Progressive Bookkeeping, Dead Drop)
- [x] Create `wip/` directory for Storm Sessions
- [x] Create Cursor rule for co-creation behaviors (`.cursor/rules/co-creation.mdc`)
- [x] Add conventions to working_style.md (Isolation, Append-Don't-Replace)
- [x] Add Close-out compound behavior under Dead Drop
- [x] Add Troubleshoot behavior for structured debugging and knowledge capture
- [x] **OpenBao + Tailscale Integration**: Go plugin for dynamic Tailscale auth keys — repo at https://github.com/SamPlaysKeys/openbao-plugin-secrets-tailscale

---

## Ideas / Future

*Capture ideas here before promoting them to "In Progress"*

- [ ] **Unattended Run Methods for AI Tools** — compare `--allow-all on`, "autopilot", "YOLO", and other unattended/headless execution modes across different AI coding assistants; evaluate tradeoffs (safety vs speed, human oversight vs autonomy), use cases (CI pipelines, batch refactors, background tasks), and suggested patterns for responsible use

- [ ] Review recovery environment — audit current backup/restore strategy, identify gaps, document recovery procedures
- [ ] ScaleTail + Vault example — create a ScaleTail sidecar container example for HashiCorp Vault deployment; demonstrate secure secrets access over Tailscale without advertising a service
- [ ] Tags/categories for troubleshooting docs findability — as `docs/troubleshooting/` grows, may need metadata or naming conventions to help locate past fixes by symptom, system, or error type
- [x] Auto-load working_style for CLI agents — Updated AGENTS.md to direct agents to follow [[working_style]] conventions.
- [ ] **AAP Controller Installation & Configuration** — Document AAP Controller setup for OpenShift Virtualization migration workflows; includes installation, licensing, execution environments, and initial configuration
- [ ] **ACM Hub-to-Managed-Cluster Topology** — Document ACM topology setup for multi-cluster OpenShift Virtualization environments; includes Hub installation, cluster enrollment, klusterlet configuration, and network requirements
- [ ] Journal in docs — create a `docs/journal/` area for capturing ideas, observations, and thoughts while working; less structured than Storm Sessions, more persistent than scratch files
- [ ] NFC integration patterns — capture as backlog items:
  - Consumable tags for replacement items (air filters): tag links to HA entity, product page, or reorder action
  - Fridge receipt launch: NFC opens receipt-scan app to eliminate drop-zone friction
  - Router/location status tags: local offline-safe status page on tap for quick service checks
  - Pi-hole temporary ad-block disable: tag triggers time-limited disable-for-30-min action with visible countdown

## Talks

*Conference talks, meetups, and presentations to develop*

- [ ] **How to Plan Out a Homelab** — talk on requirements gathering, hardware selection, and maintainability (`docs/talks/how-to-plan-a-homelab-proposal.md`)
- [ ] **Social Engineering Your Project To Success** — engaging stakeholders via social engineering, sales, and teaching concepts (`docs/talks/social-engineering-your-project-to-success-proposal.md`)

---

## Writing Ideas

*Blog posts, articles, and shareable concepts to develop*

- [ ] **Restoring vs Rebuilding** — philosophy on recovery strategies; when to restore from backup vs rebuild from scratch; tradeoffs of each approach
- [ ] **CI/CD & GitOps for Homelabs** — turn `docs/blog/cicd-gitops-homelab-brainstorm.md` into a post; covers declarative vs imperative, pull vs push CI/CD, K8s vs Docker tradeoffs, and the Komodo decision
- [ ] **The State of Podman Desktop on macOS** — blog article on Podman Desktop as a Docker Desktop alternative; source material in `docs/guides/dev-environment/podman-desktop-macos.md` and `docs/guides/dev-environment/podman-machine-krunkit-abort-trap.md`. Angle: install-path tradeoffs (DMG vs Homebrew), libkrun/krunkit gotchas, rootless vs rootful, Docker compatibility and socket mapping, and where Podman still falls short of Docker Desktop (compose edge cases, tooling friction). Target: practitioners evaluating or already running Podman on Apple Silicon.
- [ ] **Integrating HashiCorp Vault + Docker with Tailscale** — secure secrets management in a homelab context; Vault deployment patterns, Docker integration, Tailscale for secure access
- [ ] **Securing AI Workloads to Mitigate Tool-Averse Behavior** — sandboxing and guardrails that enable AI agents to act confidently; reducing hesitation through proper isolation
- [ ] **"Scratch Pad" Workspace Concept** — write up this workspace approach as a shareable concept for others who want a collaborative AI workspace; frame it as a "scratch pad" for brainstorming, troubleshooting, and building with AI as a supporting tool

## Active Sessions

# Dead Drop — 2026-07-20

**In progress:** Nothing active — session complete.

**Just completed:**
- Researched HCL BigFix setup and architecture.
- Drafted and finalized a comprehensive BigFix setup and planning guide at `docs/guides/bigfix/planning.md`, which includes architecture reviews, sizing constraints, prerequisites, installation sequence, and a consultant discovery checklist.
- Added HCL BigFix to `docs/guides/README.md` and indexed the guide.
- Logged guide creation in `BACKLOG.md`.

**Next step:** Conduct the client discovery meeting using the checklist, then proceed to the licensing/masthead creation stage when the client is ready.

**Key decision:** Chose to construct the central server planning around a DNS CNAME alias rather than hardcoded hostnames to ensure seamless future migrations.

**Git state:** uncommitted changes (modified `BACKLOG.md` and `docs/guides/README.md`, untracked `docs/guides/bigfix/`)

**Open threads:** none

# Dead Drop — 2026-06-10

**In progress:** Nothing active — session complete.

**Just completed:**
- Selected Fedora Server as baremetal Docker host OS (ADR 0002)
- Compared rootful vs rootless Docker vs Podman container runtimes (ADR 0003)
- Decided: **rootful Docker** with containerized Periphery provisioned via Ansible
- Created Ansible role structure in ADR 0002: `komodo-periphery` role with Jinja2 config template, `docker_container` task, playbook skeleton
- Documented 6 gotchas for containerized Periphery (SELinux `:z`, host networking, image tags, onboarding key rotation, config restart, vault secrets)
- Both ADRs cross-referenced and listed in ADR README

**Next step:** Build the `docker-install` and `komodo-periphery` Ansible roles for real — start with role scaffolding, Docker CE repo setup, and the Periphery container task.

**Key decision:** Flipped from rootless → rootful Docker mid-session. Rootless is more secure but Komodo's community-only support and operational friction (socket proxies, filesystem namespace issues, manual binary updates) wasn't worth the tradeoff for a homelab. Rootful + containerized Periphery is simpler to manage and Ansible-provisionable.

**Git state:** `77c6201` — uncommitted: both ADR files modified

**Open threads:** None

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
- Created `.cursor/rules/co-creation.mdc` Added Dead Drop behavior, Close-out compound behavior, and Conventions (Isolation, Append-Don't-Replace)
- Added Troubleshoot behavior for structured debugging and knowledge capture

---

# Dead Drop — 2026-05-30

**In progress:** Nothing active.

**Just completed:**
- Audited & fixed `docs/homelab/network/tailscale.md` (6 issues: Docktail URL, labels, image; ScaleTail rewrite; diagram label; decisions split)
- Created `docs/homelab/network/tailscale-grants.md` with full ACL → Grants migration
- Removed `tag:host` and `tag:storage` from all grant rules per user directive — kept as informational-only tags for sorting

**Open threads:**
- IoT subnet (`10.0.50.0/24`) is still a placeholder — will change
- `tailscale-grants.md` ready for initial Tailscale policy deployment

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

---

# Dead Drop — 2026-05-29

## Recent Progress
- Standardized Tolaria frontmatter (assigning types like Note, Reference, README-Note) across all Markdown files in `docs/` and `artifacts/` to ensure accurate vault rendering.
- Overhauled UniFi firewall architecture from legacy LAN In/Out linear rules to the new Zone-Based Firewall (ZBF) matrix.
- Designed custom zones (Prod, Test, Dev, IoT, User) to map perfectly to existing VLANs to ensure the visual policy matrix remains meaningful.
- Identified and configured the correct built-in zones (External, Gateway, and Hotspot for the Guest network isolation).
- Created explicit ZBF policies for allowed paths:
  - HA to IoT management.
  - Optional User to Prod App access (for local latency bypassing Tailscale).
  - Cross-VLAN Chromecast discovery/streaming (mDNS reflector + explicit TCP ports 8008/8009).
  - Inter-environment syncs (Prod -> Test, Test -> Dev).
- Outlined explicit Drop catch-all for inter-zone isolation.
- Graduated the ZBF plan into the core documentation at `docs/homelab/network/unifi-configurations.md`.

## Next Steps
- Open UniFi Network Controller (v9.0+) and manually configure the zones and policies based on the `docs/homelab/network/unifi-configurations.md` matrix.
- Verify Home Assistant can still reach IoT devices.
- Verify phones on the `User` VLAN can cast to Chromecasts on the `IoT` VLAN.
- Verify Tailscale remains the primary overlay for administrative routing in without conflict.
- Pull in notes on API and Ingress certificate checks and turn them into a full guide.

# Dead Drop — 2026-06-01

**In progress:** Consolidating agent behaviors into a skill-based architecture and a portable working style.

**Just completed:**
- Graduated 7 core skills (`ideate`, `troubleshoot`, `document`, `handoff`, `start`, `consolidate`, `cross-link`) from `wip/` to `.agents/skills/`.
- Created the "Portable Agent Working Style" in `workstyle/working_style.md` with a strict `Install` behavior guardrail.
- Cleaned `AGENTS.md` to establish strict Rules of Engagement (Alignment Checks, No Sweeps) and removed Tolaria references.
- Migrated `.cursor/rules/co-creation.mdc` to point to the new skill architecture.

**Next step:** Test the new behaviors in a real task, or test deploying the portable installer to a new repository.

**Key decision:** Switched from a list of loose behaviors to a strict, templated skill suite backed by an "Alignment Checks" rule to prevent the agent from "running off" without explicit permission.

**Git state:** Uncommitted changes (new skills in `.agents/skills/`).

**Open threads:** None.

# Dead Drop — 2026-06-07

**In progress:** Nothing active — session complete.

**Just completed:**
- Reviewed and verified OpenBao Tailscale plugin (`wip/openbao-tailscale-integration/`) — code is sound, ready for testing
- Ran `go mod tidy` — committed `go.mod` + `go.sum` with Go 1.24 toolchain
- Updated Dockerfile base to `golang:1.24-alpine` to match `go.mod`
- Created 5 deployment templates: inline (Dockerfile), sidecar (Dockerfile + Compose), basic Compose, K8s operator
- Created `BRIEF.md` consolidating the session
- Graduated work item in BACKLOG.md to Completed
- Plugin lives at https://github.com/SamPlaysKeys/openbao-plugin-secrets-tailscale

**Next step:** Manual integration test with `bao server -dev` and a real Tailscale API key. If that works, tag a v0.1 release.

**Key decision:** No TTL/lease needed — Tailscale owns the key lifecycle server-side. No revocation support for v0.1 (can add via key ID + DELETE API later).

**Git state:** `5d84568` — Added the new plugin link (clean working tree)

**Open threads:**
- Separate Storm Session for Tailscale IDP/OIDC auth (`wip/tailscale-idp-openbao/`) — still in progress
- Plugin v0.1 tag + release pending successful manual test

# Dead Drop — 2026-07-16

**In progress:** OpenShift Virtualization Migration Strategy — Storm Session (`wip/openshift-virtualization-VM-migration/`)

**Just completed:**
- Created execution guides for 3 orchestration paths: Ansible Core, AAP, AAP+ACM
- Created ADR 0001 for VM policy thresholds with memory/dirty-rate classification
- Created `migration-timeout-calculation.md` with calibration methodology
- Created `artifacts/` directory with 15 skeleton files (playbooks, job templates, ACM policies, manifests)
- Updated README with artifacts section and references

**Next step:** Session ready to graduate to documentation or continue with:
- Drafting actual Ansible playbooks (fill in the skeletons)
- Test matrix for Phase 2 validation
- Dedicated migration network design (Multus/NMState)

**Key decisions:**
- Timeout values are environment-dependent (calibrate per network, not fixed defaults)
- Artifacts are skeleton examples with placeholders, not turnkey solutions
- CoP deliverable needs concrete code examples alongside architecture

**Git state:** `a0d42cf` — Remove old concept files (uncommitted changes: new `artifacts/` directory, updated files)

**Open threads:**
- ADR threshold values need validation during Phase 2 testing
- Backlog items added: AAP Controller docs, ACM topology docs
