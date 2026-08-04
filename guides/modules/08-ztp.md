---
type: Guide
category: Guides
subcategory: OpenShift Learning Path
status: Active
layout: page
title: Module 8 — Zero Touch Provisioning (Specialist Track)
---

# Module 8 — Zero Touch Provisioning (Specialist Track)

> **Who this is for:** Teams managing **50+ clusters**, especially remote/edge sites where manual install isn't viable (retail, utility, telco). This is a distinct engineering specialty — Day 0/1/2 automation — beyond fleet operations. If you manage 3–20 clusters in a datacenter, **Module 6 is sufficient**; return here when provisioning becomes a bottleneck.

**Outcomes:** Automate the full cluster lifecycle — bare-metal prep through install into Day-2 — via the GitOps ZTP pipeline (SiteConfig, PolicyGenTemplate, TALM, ClusterGroupUpgrade, ClusterCurator/AAP, ESO).

**Depends on:** Modules 3–6 complete. ZTP uses Pods, operators, ACM policy, and Argo CD Applications internally.

---

## Checklist — work through in order

> Specialist track — complete Modules 3–6 first. This is Day 0/1/2 automation for 50+ cluster / edge fleets.

- [ ] Read the Day 0 / 1 / 2 framework below and the ZTP pipeline diagram
- [ ] **RHACM edge computing / ZTP** — [ZTP at scale (OCP edge computing)](https://docs.redhat.com/en/documentation/openshift_container_platform/4.19/html/edge_computing/ztp-deploying-far-edge-clusters-at-scale)
- [ ] **Day 1 install** — author a `SiteConfig` (+ `AgentClusterInstall`, `infraenv`, `NMStateConfig`); understand Assisted Installer + ApplicationSet-per-cluster
- [ ] **Day 2 config** — author a `PolicyGenTemplate`; orchestrate with **TALM** via [`ClusterGroupUpgrade`](https://docs.redhat.com/en/documentation/openshift_container_platform/4.19/html/edge_computing/cnf-talm-for-cluster-updates) (canary waves)
- [ ] **Updating GitOps ZTP** — [updating the ZTP pipeline](https://docs.redhat.com/en/documentation/openshift_container_platform/4.19/html/edge_computing/ztp-updating-gitops)
- [ ] **Secrets** — wire ESO/vault for pull-secrets, BMH creds, certs (never in Git)
- [ ] Complete the scenario-based verification at the bottom
- [ ] <!-- ORG-SPECIFIC: our ZTP Git repo, hub cluster, vault backend, hardware/canary plan -->

## Day 0 / 1 / 2 framework

| Day | Covers | ZTP component |
|-----|--------|---------------|
| **Day 0** | Pre-install: network, bare-metal prep, health checks | ClusterCurator + AAP hooks |
| **Day 1** | Install & initial provisioning | SiteConfig, AgentClusterInstall, infraenv, Assisted Installer |
| **Day 2** | Config, upgrades, compliance, scaling | PolicyGenTemplate, TALM, ClusterGroupUpgrade |

## The ZTP pipeline

```
Git commit (SiteConfig / PolicyGenTemplate)
   → Argo CD (hub) applies CRs to ACM
   → ACM + Assisted Installer provisions cluster (Day 1)
   → TALM applies Day 2 policies via ClusterGroupUpgrade
   → Managed cluster: running, compliant, GitOps-managed
```

- **Day 0** — ClusterCurator pre/post hooks run AAP jobs (network/storage/firmware checks).
- **Day 1** — `SiteConfig` (+ `AgentClusterInstall`, `infraenv`, `ManagedCluster`, `NMStateConfig`) committed to Git; ApplicationSet auto-creates an Application per cluster directory.
- **Day 2** — `PolicyGenTemplate` produces ACM policies; TALM orchestrates via `ClusterGroupUpgrade` (concurrency, canary, timeout). Upgrades follow the same Git-commit pattern.

**Upgrade strategy** depends on hardware: TALM in-place canary waves (no spare capacity), serial/parallel blue-green (spare pool or fast provisioning). Canary designation is the primary risk control for SNO fleets.

**Secrets** — ESO pulls pull-secrets, BMH creds, ingress certs, Htpasswd, ACM tokens from a vault at runtime. Never in Git.

## Official

- [ZTP at scale (OCP 4.19 edge computing)](https://docs.redhat.com/en/documentation/openshift_container_platform/4.19/html/edge_computing/ztp-deploying-far-edge-clusters-at-scale)
- [Updating GitOps ZTP](https://docs.redhat.com/en/documentation/openshift_container_platform/4.19/html/edge_computing/ztp-updating-gitops)
- [TALM for cluster updates](https://docs.redhat.com/en/documentation/openshift_container_platform/4.19/html/edge_computing/cnf-talm-for-cluster-updates)

<!-- ORG-SPECIFIC: our ZTP Git repo, hub cluster, vault backend, and hardware/canary plan. -->

## Verification (scenario-based)

1. Commit a new `SiteConfig` directory; trace: ApplicationSet → Application → ACM ManagedCluster → Assisted Installer.
2. Write a `PolicyGenTemplate` installing the Virt operator on `virt=enabled` clusters; apply via TALM `ClusterGroupUpgrade`; confirm on managed cluster.
3. Trace a version-bump commit end-to-end: `git push` → Argo CD → ACM → PolicyGenTemplate → TALM → CGU → ClusterVersion operator.
4. Walk the ESO flow: where the secret lives, how ESO auths to vault, when the K8s Secret appears.


