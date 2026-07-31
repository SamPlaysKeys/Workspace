# Plan: HCL BigFix Agent Component (node-level DaemonSet)

## Goal

Deploy the HCL BigFix client to OpenShift nodes as a privileged **DaemonSet**
running a **container image**, wired into the repo's three-tier ArgoCD
App-of-Apps. Deploy to **lab first**. Credentials pulled from **Vault via
ExternalSecret**. The exact BigFix config surface (masthead / license / relay)
is stubbed as parameterized values to finalize later.

## New component: `components/hcl-bigfix/`

Modeled on `components/etcd-backup/` (the repo's privileged host-access pattern).
Files to create:

1. **`Chart.yaml`** — `name: hcl-bigfix`, `version: 1.0.0`, no dependencies
   (plain chart).
2. **`values.yaml`** — parameterized surface:
   - `namespace.name: hcl-bigfix`
   - `image` (repo / tag / pullPolicy) — placeholder for HCL / internal registry
     image
   - `daemonset` tunables: `updateStrategy`, `tolerations` (all nodes incl.
     masters via `effect: NoSchedule, operator: Exists`), resources
   - `bigfix:` config block **stubbed** (relay FQDN, port 52311, masthead path)
     — to be filled in later
   - `commonLabels` / `commonAnnotations` / `deleteResources` hooks
3. **`templates/_helpers.tpl`** — copy verbatim from etcd-backup
   (`commonLabels` / `commonAnnotations`).
4. **`templates/namespace.yaml`** — with `openshift.io/cluster-monitoring: "true"`
   + helpers.
5. **`templates/sa.yaml`** — dedicated ServiceAccount.
6. **`templates/cr.yaml`** + **`templates/crb.yaml`** — ClusterRole granting `use`
   on the `privileged` SCC (etcd-backup Pattern A), bound to the SA.
7. **`templates/daemonset.yaml`** — the core workload:
   - `hostPID: true`, `hostNetwork: true`, `hostPath` mount of `/` (host access
     to install / register the client on the node)
   - `securityContext.privileged: true`, `runAsUser: 0`, `SYS_CHROOT` capability
   - tolerations for all nodes (masters included), broad / no nodeSelector
   - `updateStrategy: RollingUpdate`
8. **`templates/externalsecret.yaml`** — pulls BigFix creds (license / masthead)
   from Vault (`secretStoreRef: vault-cluster` / `ClusterSecretStore`), key
   `{{ .Values.cluster.name }}/bigfix_...`. **Exact keys / properties stubbed**
   pending config decision.

## Wiring changes

9. **`bootstrap/helm-values/applications.yaml`** — register under
   `availableApplications:`:
   ```yaml
   hcl-bigfix:
     annotations:
       argocd.argoproj.io/sync-wave: '3'
     source:
       path: components/hcl-bigfix
   ```
10. **`groups/lab/values.yaml`** — enable under `components-lab:`:
    ```yaml
    hcl-bigfix: {}
    ```
11. **`groups/base/values.yaml`** — add `hcl-bigfix` (its namespace) to
    `external-secrets-instance.vault.allowedNamespaces` so Vault issues the
    secret.
    - Note: since we're enabling in `lab` (not `base`), the retrofit-coverage
      gotcha does **not** apply. If later promoted to `base`, we must also add the
      `<<: *disableAutomation` entry in `groups/retrofit-no-automation/values.yaml`
      and run `scripts/validate-retrofit-coverage.sh`.

## Validation (before pushing)

12. `helm template` the applications chart with lab values + `helm template
    components/hcl-bigfix` to confirm it renders.

## Deferred (to finalize later)

- Exact BigFix agent image reference (HCL vs internal registry, pull secret).
- The precise config the client needs (masthead file, license, relay FQDN /
  port) → finalizes `values.yaml` `bigfix:` block + `externalsecret.yaml` keys.
- Whether the container just runs the client vs. installs to host — affects the
  exact daemonset command / volume mounts.

## Open questions to confirm before build

- OK to name the namespace / component **`hcl-bigfix`**?
- OK to scaffold the DaemonSet + ExternalSecret with **stubbed placeholders** for
  the BigFix-specific config, so we have a working, renderable skeleton now and
  fill in the real image / config later?
