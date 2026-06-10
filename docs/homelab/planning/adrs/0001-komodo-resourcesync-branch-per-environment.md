---
type: ADR
Status: Accepted  
Date: 2026-05-12  
---

# ADR: Per-environment Git branches via Komodo ResourceSync

## Context

The homelab uses **Komodo** with **ResourceSync** to declare servers, stacks, deployments, and related resources from Git (an “app-of-apps” style flow: Git remains the audit trail; Komodo reconciles).

Goals:

1. **`main` shows the whole picture** — `dev/`, `test/`, and `prod/` (or equivalent `komodo/{dev,test,prod}/`) layouts exist together on `main` so reviews and long-lived docs match reality after merges.
2. **Per-environment steering** — each environment’s sync can **follow `main`** or **follow a named branch** (for example to soak a stack change in Dev before it lands on `main`).
3. **Homelab-realistic ergonomics** — accept some manual merge hygiene instead of building full release automation up front.

An earlier sketch of this approach lived in `wip/komodo-artifact-templates/scratch/branch_management_ideas.md`.

## Decision

1. **Single infrastructure repo; `main` is the default line of truth**  
   After work merges, `main` holds the canonical layout for all managed environments (folder-separated configs, per existing repo structure docs).

2. **One ResourceSync configuration per Komodo-managed environment** (Dev, Test, Prod — or whichever environments are GitOps-managed)  
   Each `[[resource_sync]]` (or equivalent Komodo sync configuration) sets, at minimum:

   - **Repository** (and git provider / account as required).
   - **Branch** (or other ref field supported by Komodo’s ResourceSync schema — see [Resource sync config](https://docs.rs/komodo_client/latest/komodo_client/entities/sync/type.ResourceSync.html)) **independently per environment**.
   - **Resource path scope** limited to that environment’s subtree (for example only `komodo/dev/**` for Dev), so a branch used for experiments does not accidentally apply Prod definitions.

3. **Branch-testing workflow**  
   - Create a **feature branch** for the change under test.  
   - Point the **target environment’s** ResourceSync at that branch (for example Dev follows `feature/xyz` while Test and Prod continue to follow `main`).  
   - After validation, **merge** the feature branch to `main`.  
   - **Manually reconcile** the ResourceSync metadata on `main` so environments that should track `main` again have their branch/ref set back to `main` (or another agreed default).

   Known quirk (accepted): the ResourceSync TOML that sets `branch = "feature/xyz"` **lives in Git**. On the feature branch that file typically **must** reference the same branch so the sync pulls a self-consistent tree. After merge to `main`, that same file’s branch field must be updated to `main` (or removed to default) for environments that should no longer track the feature branch. This is intentional **small, explicit edits on `main`**, not hidden automation, until/unless you add tooling.

4. **Releases / tags as a pin for Prod (optional, not required now)**  
   Using **tags or GitHub Releases** so Prod tracks `refs/tags/v…` while `main` moves forward is a valid pattern when you want Prod immutability without branch gymnastics. With **separate per-environment directories** and per-sync branch control, the urgency is lower than in a single-shared-path model. **We defer** mandatory release-based promotion; revisit if `main` churn becomes painful for Prod.

## Consequences

**Positive**

- Clear mental model: folders = promotion path; **branch on ResourceSync** = “which revision of that tree is live for this environment.”
- Feature work can soak on Dev/Test without forking the whole repo layout conceptually on `main`.
- Decisions stay in Git and stay reviewable.

**Negative / risks**

- **Merge hygiene:** forgetting to repoint a ResourceSync after merge can leave an environment tracking a stale or abandoned branch.
- **Self-consistency:** sync config and synced paths must agree on branch, or behavior is confusing during transitions.

**Mitigations (recommended, non-blocking)**

- Short **checklist** when merging infra PRs: “ResourceSync branches updated for Dev/Test/Prod as intended?”
- Keep ResourceSync definitions in **small, dedicated files** (for example under `komodo/sync/` or similar) so diffs are obvious.
- Optionally add CI later (grep/assert expected branch for Prod) if mistakes recur.

## Alternatives considered

| Alternative | Why not chosen (for now) |
|-------------|-------------------------|
| **Prod pinned only by releases/tags** | Valuable later; deferred to reduce moving parts until branch steering is proven in daily use. |
| **Separate repos per environment** | Duplicates structure; conflicts with single-repo promotion story already documented. |
| **Only ever sync from `main`** | Forces feature testing via copy/paste or temporary files outside Git flow; rejected. |

## References

- Komodo: [Sync resources](https://komo.do/docs/automate/sync-resources)
- Homelab repo layout: [Planned repository structure](../../platform/planned-repo-structure.md) (includes promotion flow between environment folders)
- Prior scratch note: `wip/komodo-artifact-templates/scratch/branch_management_ideas.md`
