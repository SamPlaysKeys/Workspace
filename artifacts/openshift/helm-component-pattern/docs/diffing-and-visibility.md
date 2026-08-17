# Diffing and Visibility

## How Changes Flow

```
Git push (main)
  │
  ▼
GitHub Action: render-hub-applications.yml
  ├── Renders hub-bootstrap chart for every hub in clusters.yaml
  ├── Commits result to hub/rendered/hub-applications.yaml
  └── Pushes to main (if changed)
        │
        ▼
  ArgoCD detects diff on Application hub-clusters-<hubName>
        │
        ▼
  ArgoCD re-syncs hub-clusters chart with updated valueFiles
        │
        ▼
  Application spec.source.helm.values changes propagate
  to child Applications (per-component, per-cluster)
```

## Three Visibility Levels

### 1. Hub Level (`hub-clusters-<hubName>` Application)

Shows the aggregated state of all clusters on a hub. The diff here shows:
- Added/removed clusters (change in `valueFiles` list)
- Group value changes
- Cluster value changes

This is the **primary** ArgoCD view for platform engineers.

### 2. Cluster Level (Individual Applications)

Each cluster-component Application (e.g., `site-dc1-cert-manager`) shows:
- The values passed to the component chart
- Sync status and health of that component on that specific cluster

### 3. Component Level (The component chart's own resources)

Inside the target cluster, each component chart creates Namespace → OperatorGroup → Subscription → CR.
These are visible in the OpenShift console or `oc get` commands on the target cluster.

## Rate-Limiting Diffs

The hub-clusters chart uses `ignoreMissingValueFiles: true`. This means:
- Adding a group value file that doesn't exist yet won't break the render.
- Removing a group's values file from the file system won't cause a diff until you also remove the group from `clusters.yaml`.

## Common Diff Patterns

| What Changed | Diff Location | What You See |
|---|---|---|
| New cluster added to clusters.yaml | hub-clusters hub Application | New valueFile in spec.source.helm.valueFiles |
| Component version bumped in group values | hub-clusters hub Application | Changed values passed to child Applications |
| Cluster metadata updated (server, labels) | hub-clusters hub Application | Changed cluster block in child Application values |
| Component values overridden per cluster | hub-clusters hub Application | Merged values in child Application |
| Component chart template changed | Individual cluster Application | Changed rendered resources on target cluster |
