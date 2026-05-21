# Backlog

Human-in-the-loop review and tracking of what has been done and what is in progress.

> **Purpose**: This file is for high-level tracking of work items that benefit from human review and prioritization. It's the place to capture ideas, track progress on larger initiatives, and mark things complete. Unlike automated task tracking, this is intentionally manual to keep a human in the loop.

---

## In Progress

- [ ] **Homelab — public status site** (`status.samplayskeys.com`): VPS-hosted, user-facing component status; see `docs/homelab/observability/status-and-operator-dashboard.md`
- [ ] **Homelab — operator single pane:** lab overview + links (Uptime Kuma, Komodo, dynamic management URLs); see same document
- [x] **OpenShift vGPU + MTV combined program:** Host prep, VDDK pinning, and ClusterPolicy definition (Step 5 complete)
- [ ] Populate `artifacts/` with reusable automation components
- [ ] Create initial skills in `workstyle/skills/`
- [ ] Set up pre-commit hooks for secret scanning

---

## Completed

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

---

## Ideas / Future

*Capture ideas here before promoting them to "In Progress"*

- [ ] Review recovery environment — audit current backup/restore strategy, identify gaps, document recovery procedures
- [ ] ScaleTail + Vault example — create a ScaleTail sidecar container example for HashiCorp Vault deployment; demonstrate secure secrets access over Tailscale without advertising a service
- [ ] Tags/categories for troubleshooting docs findability — as `docs/troubleshooting/` grows, may need metadata or naming conventions to help locate past fixes by symptom, system, or error type
- [x] Auto-load working_style for CLI agents — Updated AGENTS.md to direct agents to follow [[working_style]] conventions.
- [ ] Journal in docs — create a `docs/journal/` area for capturing ideas, observations, and thoughts while working; less structured than Storm Sessions, more persistent than scratch files

---

## Writing Ideas

*Blog posts, articles, and shareable concepts to develop*

- [ ] **Restoring vs Rebuilding** — philosophy on recovery strategies; when to restore from backup vs rebuild from scratch; tradeoffs of each approach
- [ ] **CI/CD & GitOps for Homelabs** — turn `docs/blog/cicd-gitops-homelab-brainstorm.md` into a post; covers declarative vs imperative, pull vs push CI/CD, K8s vs Docker tradeoffs, and the Komodo decision
- [ ] **Integrating HashiCorp Vault + Docker with Tailscale** — secure secrets management in a homelab context; Vault deployment patterns, Docker integration, Tailscale for secure access
- [ ] **Securing AI Workloads to Mitigate Tool-Averse Behavior** — sandboxing and guardrails that enable AI agents to act confidently; reducing hesitation through proper isolation
- [ ] **"Scratch Pad" Workspace Concept** — write up this workspace approach as a shareable concept for others who want a collaborative AI workspace; frame it as a "scratch pad" for brainstorming, troubleshooting, and building with AI as a supporting tool
