# Convergence

## What "Converged" Means

For the helm-component-pattern, convergence is when every cluster on every hub has all of its required components installed and healthy.

Convergence is not a binary state — it has milestones:

| Milestone | What It Means | Typical Timing |
|---|---|---|
| **Hub converged** | All hub-clusters Applications are Synced and Healthy | Minutes after any clusters.yaml change |
| **Cluster discovered** | Cluster's Applications appeared in ArgoCD (Synced) | Minutes after hub converges |
| **Operators installed** | All OLM Subscriptions on the cluster report installed CSV matches desired CSV | 1-15 min per operator (depends on image pull) |
| **Instance CRs active** | All instance-level CRs (NMState, NFD, ClusterPolicy) are Healthy | 1-5 min after operator is ready |
| **Full convergence** | All components on all clusters are Synced + Healthy | Typically 5-20 min after the initiating commit |

## Monitoring Convergence

### At a glance (ArgoCD UI)

Check the hub-clusters Application for each hub. If it's Synced + Healthy, all component Applications are running. Drill into individual Applications for per-cluster status.

### Programmatically (argocd CLI)

```bash
# Check all Applications managed by hub-clusters
argocd app list -l managed-by=hub-clusters --hub <hub-name>

# Check sync status per hub
argocd app get hub-clusters-<hub-name>
```

### Alerts (Prometheus / Alertmanager)

ArgoCD exposes metrics for Application sync status. Alert on:

```
# Any Cluster App out of sync for > 15 min
argocd_app_info{sync_status!="Synced", app=~".*-cert-manager|.*-nmstate-.*|.*-nfd-.*|.*-gpu-.*"} > 0
```

## What Prevents Convergence

| Issue | Symptom | Remediation |
|---|---|---|
| Cluster unreachable | Hub Application shows `OutOfSync` or `Missing` for cluster's apps | Check cluster API health, network, ArgoCD cluster secret |
| OLM catalog source unavailable | Subscription stuck at `Updating` | Verify CatalogSource pods in openshift-marketplace |
| Operator CSV not found | Subscription reports `BadSuffix` or `NoSuchCSV` | Update startingCSV in group/cluster values |
| Instance CR rejected by webhook | CR shows `Degraded` or error events | Check CR spec against operator docs |
| GPU operator driver build fails | ClusterPolicy reports `Degraded` | Check worker node kernel headers, proxy settings |
| Low resource capacity | Pods stuck `Pending` | Scale nodes or reduce components |

## Self-Healing

This pattern uses `selfHeal: true` on all Applications by default. If a resource drifts from its desired state (deleted manually, mutated by an operator), ArgoCD restores it within the sync interval (default 3 min).

Exceptions:
- **Instance CRs** (NMState, NFD, ClusterPolicy) are self-healed by their respective operators, not directly by ArgoCD. If the operator rewrites the CR, ArgoCD may see a diff and re-apply. This is expected for operators that manage their own status subresources.
- **Disable selfHeal per component** by setting `syncPolicy.automated.selfHeal: false` in the component's registry entry or app values.
