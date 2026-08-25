---
type: ADR
Status: Draft
Date: 2026-08-25
---

# ADR: Orchestration Model — Docker-native vs Kubernetes/ArgoCD for the Homelab

## Context

The homelab currently standardizes on **Docker** (rootful, on Fedora Server baremetal hosts) managed through **Komodo** for GitOps. The prior narrative decision (*"Komodo over Kubernetes (K3s + ArgoCD)"* in `planning/decisions.md`) chose Docker-native Komodo over a K3s + ArgoCD stack on the grounds of complexity, risk tolerance, and concept alignment with existing familiarity with compose files.

This ADR reopens that axis. Two things have shifted the picture:

1. **Declarative lifecycle is now a hard requirement, not a nice-to-have.** The goal is to *easily and declaratively spin up / tear down / manage resources* across environments. That is exactly what native GitOps (ArgoCD reconciling manifests) does out of the box — and what Komodo does with more manual steering (ResourceSync branch gymnastics, per-environment TOML edits on merge, see ADR 0001).
2. **ArgoCD is the more common / more documented route than Komodo.** If the operational surface is "declare desired state, let a controller reconcile," ArgoCD is the industry-default implementation. Komodo gets us most of the way but is a smaller ecosystem with rougher edges (Podman compat, stack-status quirks).

So the earlier "Kubernetes is overkill for a homelab" conclusion is being stress-tested against a simpler question: **is the thing we're optimizing for *simplicity of the runtime* or *simplicity of declarative management*?** Those pull in different directions.

## The tension

| Axis                             | Docker + Komodo                                     | Kubernetes + ArgoCD                                                                                  |
| -------------------------------- | --------------------------------------------------- | ---------------------------------------------------------------------------------------------------- |
| Runtime complexity               | Low; Compose, one socket, familiar                  | High; control plane, CRDs, more failure modes                                                        |
| Declarative spin up/down         | Possible, but needs branch/merge hygiene (ADR 0001) | Similar; `kubectl apply` / Git commit, controller reconciles, but follows a similar git architecture |
| Single pane of glass             | Komodo dashboard (Docker-only)                      | ArgoCD dashboard (K8s-only)                                                                          |
| Ecosystem / docs                 | Smaller (Komodo)                                    | Ubiquitous (ArgoCD, CNCF)                                                                            |
| Operational burden               | Lower set-up complexity, higher per-change ceremony | Higher standing complexity, lower per-change friction                                                |
| Skills transfer to work with OCP | Dev OCP cluster already covers this separately      | Direct overlap with Red Hat stack                                                                    |

> [!INFORMATION]
> The salient counterpoint to the original decision: **Docker is less complex to *run*, but it can mean *more steps to manage* compared to native GitOps.** The "overkill" argument was about runtime footprint; it does not directly address declarative management ergonomics, which is now the priority.

## Options considered

### Option A: All Docker-native (Komodo)
Standardize everything on Docker + Komodo ResourceSync.
- **Pros:** lowest runtime complexity; builds on existing Compose investment; Dev OCP cluster still provides separate K8s skills.
- **Cons:** declarative management is real but requires the merge-hygiene rituals from ADR 0001; smaller ecosystem; no first-class "one command spins up the whole graph."

### Option B: All Kubernetes (K3s/K8s + ArgoCD)
Replace Komodo with a K3s or K8s control plane and ArgoCD as the GitOps controller for every workload.
- **Pros:** single, ubiquitous declarative model; ArgoCD is the common, well-documented path; spin up/down is a Git commit; one pane of glass for everything.
- **Cons:** real operational complexity (control plane, CRDs, more moving parts); the original "overkill for homelab" risk is still valid; more failure modes to own 24/7.

### Option C: Split model (leading direction, *not yet decided*)
Use **Docker for "prod apps"** and **Kubernetes/ArgoCD for "workload apps."**
- Keeps the low-friction Docker path for stable, user-facing services (Plex, etc.) where runtime simplicity matters most.
- Puts the heavier / more dynamic / more experiment-driven workloads on K8s/ArgoCD where declarative reconciliation pays off and aligns with work skills.
- **The only clear advantage of "all Kubernetes" (Option B) over this split is the single pane of glass** — ArgoCD as the one view of everything. That is a real UX win, but it is currently the *only* decisive argument for unifying onto K8s, and it may not outweigh the standing complexity cost.

## Decision

**No conclusion yet, this ADR is intentionally left open.**

The leading direction is **Option C (split model)**, driven by the desire for easy declarative management *without* forcing the entire homelab onto a K8s control plane it may not need. The single strongest argument for abandoning the split and going all-in on Kubernetes (Option B) is the unified ArgoCD dashboard; that has not yet been judged worth the operational cost.

What would move this from Draft to a decision:
- A concrete definition of which apps count as "prod apps" (Docker) vs "workload apps" (K8s); the boundary is currently intuitive, not enumerated.
- A validated K3s + ArgoCD bring-up path on the homelab hardware (the original decision flagged K3s-in-prod as risky; that risk needs a fresh look).
- A judgment on whether "single pane of glass" is worth the control-plane burden, or whether two dashboards (Komodo + ArgoCD) is acceptable.

## Consequences (potential)

**If Option C is adopted:**
- Two management planes to operate (Komodo for Docker, ArgoCD for K8s) — more tooling surface, but each stays within its comfort zone.
- Prod apps keep the low-ceremony Docker/Compose lifecycle.
- Workload apps get first-class declarative reconciliation and stronger skills transfer to the work OCP stack.
- Loses the "one ArgoCD view of everything" that Option B would provide.

**If Option B is adopted instead:**
- Single declarative model and dashboard for the whole homelab.
- Pays the standing K3s operational cost the original decision sought to avoid.
- Supersedes the "Komodo over Kubernetes" narrative in `decisions.md` — that section would need revisiting.

## Open Questions

- Where exactly is the "prod app" vs "workload app" line drawn?
- Does "prod app" vs "workload app" need to be an treated as alternative approach to my originally declared "Prod/Test/Dev" environments from [ADR 0001](0001-komodo-resourcesync-branch-per-environment.md)?
	- Idea: If this is the case, it would call into question the order of evolution in regards to hardware, with "Prod" still running on a dedicated device, having "Test" running within the ProxMox cluster, and having "Workload/Dev" on a larger version of the OCP cluster.
- Is the K3s-in-prod risk from the original decision still valid, or has the operational picture changed?
- If my personal work is focused largely on kubernetes, will it be better to have everything involved with kubernetes, or will having "prod apps" in docker (option C) encourage me to not mess with prod apps and focus my time on just the kubernetes side of the field?
- Is there a "single pane of glass" option that can manage both docker and kubernetes, or at least have visibility on both?
- Does losing a single pane of glass (under the split) actually hurt day-to-day, or is it tolerable?
- Does this reopen the runtime question (ADR 0003 / 0002) for the K8s-hosted workloads, or do they stay rootful Docker-in-VM / containerd as K8s dictates?

## References

- Narrative decision: [Komodo over Kubernetes (K3s + ArgoCD)](../../planning/decisions.md) — the earlier conclusion this ADR re-examines.
- ADR 0001: [Per-environment Git branches via Komodo ResourceSync](0001-komodo-resourcesync-branch-per-environment.md) — the merge-hygiene cost of Docker-native GitOps.
- ADR 0002: [Baremetal Docker Host OS Selection](0002-baremetal-host-os.md)
- ADR 0003: [Container Runtime Tradeoffs](0003-container-runtime-tradeoffs.md)
