# Operator Management

## All Operators Use OLM

Every operator in this pattern is installed via **OLM** (Operator Lifecycle Manager) using the standard Subscription → OperatorGroup → Namespace pattern.

Components are split into two chart types:

### Operator Chart

Installs the OLM subscription. Contains:
- Namespace
- OperatorGroup
- Subscription (with channel, approval, and optional startingCSV pinned)

### Instance Chart

Creates the operator's custom resource (e.g., NMState, NodeFeatureDiscovery, ClusterPolicy) that activates the operator.

This split gives us:
- **Sync wave ordering** — Operator subscription installs in wave 0, instance CR creates in wave 5+. The operator is ready before the instance CR tries to reconcile.
- **Independent lifecycle** — You can uninstall the instance (suspend the operator's workload) without removing the operator itself.
- **Upgrade isolation** — Bump the Subscription's startingCSV independently of the CR spec.

## CSV Pinning

Every Subscription pins a `startingCSV` to a specific version. This ensures:

1. All clusters on a hub run the same operator version.
2. Upgrades are explicit — you bump the CSV in group values, not in the OpenShift UI.
3. Rollback is a Git revert.

### Upgrade Process

1. Update `startingCSV` in the relevant group values (e.g., `groups/all/values.yaml`).
2. Commit and push.
3. ArgoCD syncs the update to all affected clusters.
4. OLM handles the CSV transition (new CSV replaces old CSV).
5. Verify on a dev cluster first — use a separate dev hub with a canary group.

### Going Back (Rollback)

1. Revert the startingCSV change in Git and push.
2. ArgoCD re-applies the old startingCSV.
3. OLM rolls back to the previous CSV version (automatic via the Subscription's `installPlanApproval`).

> **Note:** CSV rollback depends on the previous CSV still being available in the catalog source. If the old CSV has been pruned (common in fast-moving catalogs), you may need to use a specific index image or accept the new version.

## Operator Group Scope

All operator groups use `targetNamespaces` set to the operator's namespace (single-namespace scope), not `AllNamespaces`. This ensures operators only watch resources in their own namespace.

## Catalog Source Resolution

Each Subscription specifies:

- `source`: The CatalogSource name (e.g., `redhat-operators`, `certified-operators`)
- `sourceNamespace`: The CatalogSource namespace (always `openshift-marketplace`)

If your disconnected environment uses custom CatalogSources, override these values in the group or cluster values.
