# Architecture Opinions

This document captures the architectural decisions and trade-offs baked into the helm-component-pattern.

## Guiding Principles

1. **Helm all the way down.** No Kustomize overlays. The entire dispatch pipeline — from hub bootstrap to per-cluster component values — is pure Helm with `--values` layering. This gives us a single rendering engine with predictable semantics: `mustMergeOverwrite` for priority stacking, and `--values` for file-level precedence.

2. **ArgoCD only, no RHACM.** This pattern uses vanilla ArgoCD Applications. It does not require Red Hat Advanced Cluster Management (RHACM). This makes it portable to any OpenShift fleet (and even non-OpenShift clusters with minimal adaptation).

3. **Group-and-cluster override model.** Every cluster belongs to one or more groups. Group values are merged in declaration order from `clusters.yaml`. The cluster's own values file merges last as the highest-priority override. This mirrors Kubernetes' own layered configuration philosophy.

4. **One Application per component per cluster.** Each component on each cluster gets its own ArgoCD Application. This gives fine-grained sync visibility, independent sync waves, and per-component rollback. The sync-wave annotations in `componentRegistry` control ordering.

5. **GitHub Action render, not ArgoCD.** The hub-bootstrap chart is rendered by a GitHub Action (not by ArgoCD itself) because its inputs (clusters.yaml, group/cluster values) are the same files that define the ArgoCD Applications. Rendering outside ArgoCD avoids the chicken-and-egg problem of needing Applications to exist before they can define themselves.

6. **clusters.yaml as the single source of truth.** Every cluster's hub assignment, groups, server URL, and metadata live in one YAML file. No per-cluster config maps or separate inventory databases.

## Non-Goals

- **Not a cluster lifecycle tool.** This pattern manages add-on components on existing clusters. Cluster provisioning (creating/destroying clusters) is outside scope.
- **No day-2 operator UI.** This is a GitOps pattern, not a management console.
- **Not for single-cluster deployments.** The pattern overhead (hub bootstrap, group abstractions) only pays off at fleet scale.

## What You Trade

For the benefits above, you accept:

- **Helm template complexity.** The `application.yaml` template in `hub-clusters` uses nested range/if/mustMergeOverwrite. Contributors need basic Helm template literacy.
- **`--values` file proliferation.** Each group and cluster adds a values file. At 50+ clusters the valueFiles list in the hub Application becomes long, but ArgoCD handles this gracefully with `ignoreMissingValueFiles`.
- **GitHub Action dependency.** The hub-bootstrap render-and-commit job is a CI dependency. If the GitHub token or workflow breaks, new clusters can't register themselves until it's fixed.
