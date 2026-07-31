# Maintainers Guide

## Responsible Person

The helm-component-pattern is maintained by **SamPlaysKeys**. Each component area has specific knowledge notes below.

### Contact

| Area | Contact | Channel |
|------|---------|---------|
| Helm chart architecture | SamPlaysKeys | GitHub Issues |
| Release management | SamPlaysKeys | GitHub Issues |
| CI/GitHub Actions | SamPlaysKeys | GitHub Issues |

### Component Knowledge Areas

| Component | Notes |
|-----------|-------|
| cert-manager | Standard OLM install |
| NMState | Instance CR may need network-level review |
| NFD | PCI whitelist must match hardware inventory |
| NVIDIA GPU Operator | Driver version, ClusterPolicy tuning |
| KubeVirt | HyperConverged CR tuning |

## Maintenance Cadence

- **Monthly** — Review open PRs, update CSV versions, prune stale clusters.
- **Quarterly** — Audit `clusters.yaml` for accuracy, review sync failures across hubs.
- **Per-release** — Bump component CSV versions in group values, test on dev hub first.

## Onboarding a New Component

1. Add a `components/<name>/` chart with the standard operator/instance split where applicable.
2. Register in `charts/hub-clusters/values.yaml` under `componentRegistry`.
3. Add group-level enabled/disable config in `groups/all/values.yaml` if the component should be fleet-wide, or in a specific group.
4. Update this document.
