# Platform component delivery at fleet scale

*Composable group-based GitOps for OpenShift fleets — ArgoCD only, no RHACM required.*

---

## Who this is for

**Platform engineers and consultants** rolling out or managing a fleet of OpenShift clusters, looking for a structured GitOps-native way to manage platform components across clusters that share most config but differ in meaningful ways.

**CoP maintainers** — this pattern builds on the conventions established in [redhat-cop/gitops-standards-repo-template](https://github.com/redhat-cop/gitops-standards-repo-template). The folder structure and composable groups model are the same; the implementation choices differ. See [docs/cop-maintainers.md](cop-maintainers.md) for a full comparison, trade-off analysis, and guidance on choosing between them.

---

## Further reading

| Document | Audience |
|---|---|
| [docs/architecture-opinions.md](docs/architecture-opinions.md) | The problem, the principle, the Hub concept, and the 9 design opinions with full trade-offs |
| [docs/cop-maintainers.md](docs/cop-maintainers.md) | CoP maintainers: relationship to gitops-standards, Helm vs Kustomize trade-offs, choosing between them |
| [docs/diffing-and-visibility.md](docs/diffing-and-visibility.md) | PR-level desired-state diffs, fleet-wide live-to-desired diffs, argocd-diff-preview integration |
| [docs/convergence.md](docs/convergence.md) | *(Aspirational)* How this pattern and gitops-standards could converge; `sourceType` per component; open questions |
| [docs/operator-management.md](docs/operator-management.md) | Operator installation via OLM, `operators-installer` integration, version pinning, operator+instance split |

---

## The problem

You have multiple OpenShift clusters. Each cluster needs a set of platform components — cert-manager, nmstate, kubevirt, logging, monitoring agents, etc. Most config is the same across clusters, but not all:

- Some clusters run OpenShift Virtualization; others don't
- Some are single-node edge clusters with reduced resource requests
- Some clusters override a specific setting (e.g. `installPlanApproval: Manual` in production)
- New clusters need to be onboarded without copy-pasting config from an existing one

You want all of this managed by GitOps: every change goes through a pull request, ArgoCD applies it, and nothing is applied by hand.

**What goes wrong without a pattern:**

```
clusters/
  site-dc1/cert-manager.yaml      ← full config copy
  site-dc1/nmstate.yaml           ← full config copy
  site-edge-1/cert-manager.yaml   ← slightly different copy
  site-edge-1/nmstate.yaml        ← slightly different copy
  site-dc2/cert-manager.yaml      ← another copy ...
```

When you want to change the cert-manager channel across all production clusters, you update N files. When you add a new cluster, you copy-paste and manually adjust. Config drifts. Reviews miss changes buried in large diffs.

---

## The principle

Define each platform component **once**, with sensible defaults. Define **groups** that describe cluster types. Assign each cluster to groups. Merge the layers in priority order — later layers win.

```
component-all           ← baseline: every cluster gets cert-manager, channel: stable
component-virt-enabled  ← override: add kubevirt, bump nmstate to stable-4.16
component-edge-sno      ← override: reduce cert-manager resource requests
component-site-dc1      ← cluster override: cert-manager installPlanApproval: Manual
```

For a cluster in groups `[all, virt-enabled]` with a cluster-specific override, the merge order is:

```
component-all  ──mustMergeOverwrite──▶  component-virt-enabled  ──mustMergeOverwrite──▶  component-site-dc1
(lowest priority)                                                                          (highest priority)
```

Each layer **deep-merges** into the previous. A key set in `component-virt-enabled` only affects that key — it doesn't replace the entire map from `component-all`. The cluster layer is always last and always wins.

ArgoCD generates one Application object per enabled component per cluster. Config is resolved at render time; no per-cluster copy-paste.

---

## "Hub" — ArgoCD fleet topology, not RHACM

This pattern uses the word **hub** to mean: *a cluster where ArgoCD runs, which deploys applications to other (spoke) clusters*.

This is standard ArgoCD fleet terminology. **This pattern requires only ArgoCD** — no Red Hat Advanced Cluster Management (RHACM) is installed or used in this reference implementation.

```
Hub cluster (ArgoCD runs here)
  ├── Deploys to: site-dc1   (spoke)
  ├── Deploys to: site-dc2   (spoke)
  └── Deploys to: site-edge-1  (spoke)
```

**RHACM can be used alongside this pattern and makes some things easier.** RHACM automates the operational steps that this pattern leaves manual:

- Registering spoke clusters with the hub ArgoCD instance (cluster secrets, RBAC, kubeconfig)
- Propagating the ArgoCD namespace and service account to spoke clusters
- Cluster lifecycle (provisioning, decommissioning, upgrades)
- Policy enforcement across the fleet independent of ArgoCD

If RHACM is available, use it for cluster registration and let this pattern handle *what gets deployed* to each cluster. The two are complementary: RHACM manages the fleet topology; this pattern manages the application configuration that runs on it.

Multiple hub clusters are supported — `prod-a`, `prod-b`, and `dev` in this example. Each hub manages a subset of spoke clusters. All hubs read from the same Git repository.

---

## Folder layout

```
helm-component-pattern/
├── clusters.yaml              # Central cluster inventory: hub, groups, server, shared attributes
├── charts/
│   ├── hub-clusters/          # Per-cluster Application + AppProject generator (live ArgoCD render)
│   └── hub-bootstrap/         # Per-hub Application generator (GitHub Action only)
├── components/                # Individual platform component charts (the deployables)
│   ├── nmstate/
│   │   ├── operator/          # OLM installation via operators-installer subchart
│   │   └── instance/          # NMState CR — activated after operator installs
│   └── cert-manager/
├── groups/                    # Composable cluster profiles
│   ├── all/values.yaml        # component-all: fleet baseline (lowest priority)
│   ├── virt-enabled/values.yaml  # component-virt-enabled: enables OCP Virt
│   └── edge-sno/values.yaml   # component-edge-sno: resource tuning for SNO
├── clusters/                  # One directory per cluster — app overrides only
│   ├── site-dc1/values.yaml   # component-site-dc1: overrides; identity lives in clusters.yaml
│   ├── site-edge-1/values.yaml
│   ├── site-dc2/values.yaml
│   └── site-dev-1/values.yaml
├── hub/
│   ├── bootstrap-root.yaml           # The ONE Application applied by hand (watches hub/rendered/)
│   └── rendered/
│       └── hub-applications.yaml     # Generated by hub-bootstrap — committed by GitHub Action
└── .github/workflows/
    └── render-hub-applications.yml   # Renders hub-bootstrap chart → commits hub/rendered/
```

---

## How the charts relate

| Chart | Run by | Input | Output |
|---|---|---|---|
| `hub-bootstrap` | GitHub Action (CI) | `clusters.yaml` | One `hub-clusters-<hub>` Application per hub — committed to `hub/rendered/` |
| `hub-clusters` | ArgoCD at sync time | `clusters.yaml` + group/cluster values | One component Application per enabled app per cluster; one AppProject per cluster |
