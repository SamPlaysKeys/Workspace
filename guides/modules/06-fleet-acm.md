---
type: Guide
category: Guides
subcategory: OpenShift Learning Path
status: Active
layout: page
title: Module 6 — Fleet Management with ACM
---

# Module 6 — Fleet Management with ACM

**Audience:** Engineers operating multiple clusters who need consistency, compliance, and declarative correction at fleet scale.

**Outcomes:** Govern a fleet declaratively with RHACM — import clusters, target via labels, report compliance, enforce or inform, and bridge ACM + Argo CD (hybrid). Understand the decision framework: *when should a config NOT be in Argo CD?*

**Prerequisite:** Modules 3 and 5. **Lab:** hub cluster with ACM + OpenShift GitOps, plus ≥1 managed cluster. Single-node lab is insufficient.

---

## Checklist — work through in order

- [ ] Read the fleet-thinking mental shift below (policy-first, not cluster-first)
- [ ] Stand up the lab: hub cluster with **RHACM** + OpenShift GitOps + ≥1 managed cluster
- [ ] **RHACM governance & policy** — [ACM governance docs](https://docs.redhat.com/en/documentation/red_hat_advanced_cluster_management_for_kubernetes/2.16/html/governance/governance); import a cluster, label it, write your first policy
- [ ] Learn **PolicyGenerator** (YAML → policies) and **Placements/PlacementBindings** for label targeting
- [ ] Practice **`inform` vs `enforce`** and the **Argo CD Application vs ACM Policy** decision table
- [ ] **DO432** (Multicluster Management with Red Hat Advanced Cluster Management for Kubernetes) → validates with **EX432**
- [ ] Complete the scenario-based verification at the bottom
- [ ] <!-- ORG-SPECIFIC: our hub cluster, fleet topology, and mandates vs team-owned configs -->

## Mental shift — fleet thinking

| Single-cluster instinct | Fleet-operator reframe |
|-------------------------|------------------------|
| "I'll fix this cluster" | "I'll fix the policy; ACM applies it everywhere" |
| "Who changed that?" | "Which commit changed the policy? Which clusters are non-compliant?" |
| "I'll deploy this operator" | "All `virt=enabled` clusters should have the Virt operator — write a policy" |
| Console change then document | Console is observe; Git is the fleet change record |

## Topics

- **Cluster lifecycle with ACM** — import, label for targeting, view fleet compliance
- **PolicyGenerator** — YAML → RHACM policies via kustomize plugin
- **Placements / PlacementBindings** — label-based targeting
- **`inform` vs `enforce`** — detect/report vs auto-remediate
- **`PolicyAutomation`** — link a compliance event to an AAP job (ServiceNow, PagerDuty, Day-2 tasks)
- **Ansible + ACM bridge** — AAP generates CRs via Jinja (Redfish/Vault/DNS); ACM deploys; policies trigger AAP post-provisioning. Idempotent.
- **Secret management** — ESO + Vault back end (prereq for Module 8)

## Key decision: Argo CD Application vs ACM Policy

| Signal | Argo CD / operators-installer | ACM Policy |
|--------|-------------------------------|------------|
| Who decides it exists? | A team owns it | Platform mandate for all of type X |
| If missing? | Workload breaks; team fixes | Compliance violation; platform must know |
| Enforcement required? | Reconciliation sufficient | Drift detected/auto-corrected |
| Environment variation? | Yes (dev `alpha`, prod `stable`) | No (same baseline) |
| Audit needed? | Git log + PR | ACM dashboard + policy report + Git log |
| Lifecycle stage | Day 2 workload | Day 1 bootstrap / org baseline |

*Use ACM Policy for:* NMState, cert-manager, Virt operator, file-integrity, OAuth, kubelet config, kubeadmin removal, pull-secret distribution. *Use Argo CD for:* team-owned operators varying by env/team.

**Grey zone:** a mandate on a heterogeneous fleet needing version variation → use **ACM policy templating** (Go-template against cluster labels), not a switch to Argo CD. The hybrid pattern: ACM *delivers* an Argo CD ApplicationSet to every cluster; ACM ensures the object exists, Argo CD reconciles content.

## Official

- [GitOps with ACM](https://docs.redhat.com/en/documentation/red_hat_advanced_cluster_management_for_kubernetes/2.16/html/gitops/gitops-overview)
- [ACM governance](https://docs.redhat.com/en/documentation/red_hat_advanced_cluster_management_for_kubernetes/2.16/html/governance/governance)
- [PolicyGenerator integration](https://docs.redhat.com/en/documentation/red_hat_advanced_cluster_management_for_kubernetes/2.11/html/governance/integrate-policy-generator)

<!-- ORG-SPECIFIC: our hub cluster, fleet size/topology, and which configs are mandates vs team-owned. -->

## Verification (scenario-based)

1. Import a second cluster; label it; write a PolicyGenerator enforcing an RBAC/alertmanager rule on that label; confirm compliance in ACM console; flip `inform`→`enforce`; observe difference.
2. Explain fleet compliance without notes: what "non-compliant" means, who's notified, monitoring vs remediation policy.
3. Scenario: Virt operator on all prod clusters (mandate) + Strimzi only on one team's clusters. Argue the tool for each; what changes if Virt becomes a regulatory requirement?


