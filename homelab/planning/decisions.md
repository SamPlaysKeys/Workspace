---
type: Reference
layout: page
title: Decisions
category: Homelab
status: Active
---
{% raw %}


# Decisions

Key architectural and tooling decisions, with rationale.

---

## Komodo over Kubernetes (K3s + ArgoCD)

**Decision:** Use Komodo for container GitOps instead of deploying K3s with ArgoCD.

**Context:** Needed GitOps capabilities (drift detection, continuous reconciliation) for Prod/Test environments. Current approach (GitHub Actions → Ansible playbooks) works but lacks:
- Drift detection
- Continuous reconciliation
- Sustainable operational model

**Options considered:**

| Option | Pros | Cons |
|--------|------|------|
| **K3s + ArgoCD** | Familiar with ArgoCD, true GitOps, industry standard | K8s operational complexity, more failure points, overkill for homelab |
| **Komodo** | Docker-native, ~2.5× less config, uses familiar Compose concepts | Newer tool, less ecosystem |
| **Ansible Pull** | Uses existing skills, simple | DIY drift detection, more manual work |
| **Portainer Business** | Has GitOps features | Paid, vendor lock-in |

**Decision rationale:**
1. **Complexity:** Komodo config is ~67 lines for a Plex stack vs ~176 lines for equivalent K8s manifests
2. **Risk tolerance:** K3s in production homelab felt risky — more moving parts, less familiar failure modes
3. **Concept alignment:** Docker Compose is already known; Komodo extends it rather than replacing it
4. **Operational simplicity:** Single Komodo Core + Periphery agents vs. K8s control plane + ArgoCD + supporting infrastructure

**Trade-offs accepted:**
- Komodo is a newer, smaller project than ArgoCD
- Less community support and fewer integrations
- If K8s skills are needed for work, the OCP cluster in Dev provides that separately

---

## Three-Tier Managed Docker Hosts (Dev → Test → Prod)

**Decision:** DevDocker VM is Komodo-managed, not a sandbox.

**Context:** Initially considered all dev nodes as unmanaged sandboxes. Reconsidered based on promotion path needs.

**Rationale:**
- DevDocker is where container work starts before going to production
- Having it in Komodo creates a consistent workflow: same TOML format, same tooling
- Promotion is literally moving a file between directories
- Other Dev nodes (VMs, OCP cluster) don't need this — they're for different purposes

---

## Dedicated Komodo Controller (NUC)

**Decision:** Run Komodo Core on dedicated hardware with no workloads.

**Rationale:**
- Management plane should be separate from workload infrastructure
- If a managed node has issues, the controller can still observe and remediate
- Clean failure domain — controller failures don't affect running workloads (they just stop being managed temporarily)
- NUC is low-power, can run 24/7 without significant cost

---

## Terraform + Ansible (Not Just Ansible)

**Decision:** Use Terraform for provisioning, Ansible for configuration.

**Context:** Already comfortable with Ansible. Question was whether to use Ansible for everything or split responsibilities.

**Boundary:**
- **Terraform:** Provision VMs, network resources, base infrastructure
- **Ansible:** System configuration, package installation, Komodo Periphery agent deployment

**Rationale:**
- Terraform has better state management for infrastructure
- Ansible is better for configuration management and idempotent system setup
- Clean separation of concerns — "what exists" (Terraform) vs "how it's configured" (Ansible)

---

## Poll-Based GitOps (Not Webhooks)

**Decision:** Use Komodo's default poll-based sync rather than GitHub webhooks.

**Context:** Komodo supports both:
- Poll-based: Core periodically checks Git for changes
- Webhook-based: GitHub notifies Core immediately on push

**Rationale:**
- Simpler setup — no webhook configuration needed
- Works even if GitHub can't reach the homelab (no public endpoint required)
- Slight delay (poll interval) is acceptable for homelab use case
- Can add webhooks later if immediate deployment becomes important

---

## Per-environment Git branches (ResourceSync)

**Decision:** Use **separate Komodo ResourceSync configurations per environment**, each with its own **Git branch / ref**, so `main` can hold all environment folders at once while Dev (or Test) temporarily tracks a feature branch for validation.

**Context:** Feature work needs to soak in a managed environment without blocking `main` or copying configs outside Git. ResourceSync already supports repo + branch + scoped paths.

**Operational trade-off:** ResourceSync TOML that lives in Git may reference the feature branch while that branch is active; after merge to `main`, **small manual edits on `main`** repoint environments back to `main` where intended. Releases/tags for Prod-only pinning are optional and deferred until needed.

**Full rationale, consequences, and mitigations:** [ADR — Per-environment Git branches via Komodo ResourceSync](adrs/0001-komodo-resourcesync-branch-per-environment.md).

---

## References

### FoxxMD — Migrating to Komodo (Nov 2024, updated Aug 2025)

**URL:** https://blog.foxxmd.dev/posts/migrating-to-komodo/#create-komodo-periphery-agents

An opinionated migration guide covering a homelab with 5 servers, 20+ stacks, and 60+ containers. Relevant to our setup given similar topology.

**What it covers:**

- **Storage strategy:** Monorepo layout for stacks (`stacks/serverN/`) and Komodo resource TOMLs (`komodo/resources/main.toml`). Git repo-based for all resources (backup via commits on save).
- **Rootless Periphery (community pattern):** Docker socket-proxy (`linuxserver/socket-proxy`) with a non-root Periphery container, controlled via `DOCKER_HOST: tcp://socket-proxy:2375`. Also covers building a custom Periphery image for git user config when running non-root.
- **Periphery deployment evolution:** Started containerized, switched to systemd agent after 3+ months — "systemd makes Docker interactions simpler." Notes terminal/shell access differences between container and systemd agents.
- **Stack creation:** Configuring Linked Repos for monorepo, `Run Directory` per stack, converting standalone containers via `docker-autocompose`.
- **Environment variables & Secrets:** How Komodo's `.env` interpolation works, secrets via `[[SecretName]]` syntax.
- **Resource Sync:** Setting up bi-directional sync between Komodo and a Git repo for full topology backup. Tags for scope limiting, Execute vs Commit modes.
- **Docker data agnostic location:** Using `$DOCKER_DATA` ENV per host for bind mount paths, making compose files portable across servers.

**Follow-up post (also by FoxxMD):** [Komodo FAQ, Tips, and Tricks](https://blog.foxxmd.dev/posts/komodo-tips-tricks/) — covers container exec shortcuts, troubleshooting, and deeper operational patterns.

{% endraw %}