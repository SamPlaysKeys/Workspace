---
type: Guide
category: Guides
subcategory: OpenShift Learning Path
status: Active
---
# Module 5 — CI/CD & GitOps

**Audience:** Engineers ready to apply Git-as-change-management to cluster configuration and application delivery.

**Outcomes:** Build CI/CD pipelines; use Argo CD (OpenShift GitOps) as the reconciliation engine; structure a multi-cluster GitOps repo; handle secrets safely; diagnose sync/drift and failure modes.

**Depends on:** Module 1 (Git) and Module 3 (cluster operations). Lab: one cluster + one Git repo is sufficient.

---

## Checklist — work through in order

- [ ] Confirm Module 1 (Git) + Module 3 (cluster ops) are done
- [ ] Read the mental model (Git is source of truth; console is observe-only)
- [ ] **Learn GitOps + Argo CD:** [Akuity Academy — Intro to GitOps & Argo CD](https://academy.akuity.io/courses/gitops-argocd-intro) (free, by the Argo creators) or KodeKloud/Coursera "Argo CD for the Absolute Beginners". Argo CD is a **CNCF Graduated** project.
- [ ] Deploy your first Argo CD Application from Git; test drift + self-heal
- [ ] Learn **ApplicationSet** and **app-of-apps**; build the `components/groups/clusters` repo structure from the [redhat-cop template](https://github.com/redhat-cop/gitops-standards-repo-template)
- [ ] Pick a **secrets** pattern (Sealed Secrets / External Secrets Operator)
- [ ] **CI side:** build a pipeline (GitHub Actions / GitLab CI / Tekton) that publishes images + manifests <!-- ORG-SPECIFIC: our CI system, registry, promotion model -->
- [ ] Pass the scenario-based Verification at the bottom

---

## Mental model

A PR merge *replaces* a change ticket. A `git revert` *replaces* an emergency rollback. **Argo CD** closes the loop: it continuously reconciles what is in Git against what runs in the cluster. The console is read/observe only — changes made there do not survive a sync.

## Topics

- **OpenShift GitOps (Argo CD)** — Application, AppProject, sync policies, health checks, self-healing
- **ApplicationSet** — generate many Applications from one template (by cluster label, directory, branch)
- **App-of-apps** — a root Application managing child Applications; full inventory lives in Git
- **Configuration drift** — what Argo CD detects; `selfHeal` vs manual sync
- **Sync options** — `Validate`, `CreateNamespace`, `RespectIgnoreDifferences`, retries
- **GitOps repo structure** — get this right early. Use the `components / groups / clusters` pattern:

```
components/   # atomic, reusable building blocks — one concern per subfolder
groups/       # composable cluster profiles (Kustomize Component, not overlay)
clusters/     # cluster-specific config — selects which groups to compose
```

Groups are Kustomize `kind: Component` (composable, not inherited) — a cluster can belong to `all + non-prod + geo-east` simultaneously. Reference [`redhat-cop/gitops-standards-repo-template`](https://github.com/redhat-cop/gitops-standards-repo-template) and the 80+ component library [`redhat-cop/gitops-catalog`](https://github.com/redhat-cop/gitops-catalog) as remote bases instead of writing operator YAML from scratch. A Helm variant uses `mustMergeOverwrite` with `component-<name>` keys.

- **Mono-repo vs multi-repo** — most teams start mono-repo, split when access control forces it.
- **Secrets in GitOps** — never commit plaintext/base64 secrets. Three patterns: (1) **Sealed Secrets** (asymmetric, in-cluster decrypt — simple); (2) **External Secrets Operator** (vault-backed, preferred for prod); (3) **Helm Secrets / SOPS** (encrypted values).

## CI/CD (pipelines)

- Build/test/publish images and manifests in CI; let GitOps deliver to clusters.
- <!-- ORG-SPECIFIC: our CI system (GitHub Actions / GitLab CI / Tekton), registry, and promotion model (dev→stage→prod). -->

## Official

- [Understanding OpenShift GitOps](https://docs.redhat.com/en/documentation/red_hat_openshift_gitops/latest/html/understanding_openshift_gitops/index)
- [`redhat-cop/gitops-standards-repo-template`](https://github.com/redhat-cop/gitops-standards-repo-template)
- [`redhat-cop/gitops-catalog`](https://github.com/redhat-cop/gitops-catalog)

<!-- ORG-SPECIFIC: our Argo CD instance URL, repo conventions, and secret-backend choice (Sealed Secrets vs ESO+vault). -->

## Verification (scenario-based)

1. Create an Argo CD Application from Git. Change replica count/image tag in Git; watch convergence. Roll back via `git revert`; watch follow.
2. Introduce drift — change a resource via `oc`, bypassing Git. Confirm Argo CD detects/remediates per sync policy; explain why the console change didn't survive.
3. Create an ApplicationSet generating one Application per directory; add a directory; confirm auto-creation.
4. **Failure modes:** push broken YAML (syntax error / missing field) — what state? How to find the sync error without console? Deploy a crashlooping pod — distinguish `OutOfSync` (differs from Git) from `Degraded` (matches Git but unhealthy); explain how both can be true.
5. **Repo structure:** clone the COP template; map each folder from `kustomization.yaml` only. Design `groups/` for `all` + `virt-enabled` + `edge-sno`; explain how a cluster in both `virt-enabled` and `edge-sno` references both via `kind: Component`.
6. **Catalog:** find NMState/MetalLB/ESO bases in `gitops-catalog`; create `components/nmstate/kustomization.yaml` referencing the catalog; point an Application at it; confirm install.
7. **Secrets:** a teammate committed a base64 `Secret`. Explain exposure, remediate with Sealed Secrets vs ESO (why), and prevent recurrence via pre-commit hook or CODEOWNERS.


