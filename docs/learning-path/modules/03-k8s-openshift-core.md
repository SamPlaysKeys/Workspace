---
type: Guide
category: OpenShift Learning Path
status: Active
---
# Module 3 — Kubernetes & OpenShift Core

**Audience:** Infrastructure/platform engineers and developers operating or deploying on Kubernetes/OpenShift. (VMware admins: this is where you rebuild the mental model from Kubernetes up — see the mapping scaffold, then discard it.)

**Outcomes:** Read and reason about YAML; navigate console + CLI; deploy and diagnose a stateless app; operate cluster-level concerns — cluster operators, MachineConfig, nodes, auth, SCCs, OLM, multi-tenancy, networking, storage, observability, and backup/DR.

---

## Checklist — work through in order

- [ ] **Prereq:** basic Linux (Red Hat RH124 or equivalent) — you will live on the CLI
- [ ] Read the *Mental shift* below and internalize desired-state / nodes-are-cattle
- [ ] **3.1 App layer** — Deployments, Services, Routes/Ingress, ConfigMaps/Secrets, RBAC, Operators
- [ ] **3.2 Cluster ops** — cluster operators, MachineConfig, nodes, OAuth, SCCs, OLM, observability, backup/DR
- [ ] **3.3 Networking** — OVN-Kubernetes, NetworkPolicy, Multus/NAD, MetalLB, NMState
- [ ] **3.4 Storage** — StorageClass, PV/PVC, CSI, ODF, RWX vs RWO
- [ ] **Courses:** KodeKloud "Kubernetes for the Absolute Beginners" / CKA / CKAD (Mumshad) for vendor-neutral depth; **DO180 → DO280** for the OpenShift track
- [ ] **Certify (optional, vendor-neutral):** CNCF **KCNA** (entry) → **CKAD** or **CKA** (performance-based, 66% to pass, $445). OpenShift track: **EX280** (Module 9)
- [ ] Pass the scenario-based Verification in each sub-section

<!-- ORG-SPECIFIC: which clusters/tracks are required vs optional for your role -->

---

## Mental shift (read first)

> **OpenShift is a Kubernetes platform. OpenShift Virtualization (Module 4) is a Kubernetes add-on that runs VMs inside Kubernetes. The substrate is Kubernetes — not a hypervisor.**

Treat vCenter/ESXi analogies as a *temporary scaffold only*. They break:
- **Console is read/observe, not source of truth** — Git is (Modules 5+).
- **Nodes are cattle** — drainable/replaceable; state lives in PVCs.
- **Manifests are desired-state** — controllers reconcile continuously; divergence is auto-corrected.

| VMware concept | K8s/OCP analogue | Discard when… |
|----------------|---------------|---------------|
| vCenter UI | OpenShift Console (observe only) | Breaks in Module 5 (GitOps) |
| ESXi host | Worker node | Breaks in Module 3.1 (nodes are cattle) |
| Resource pool / reservation | Requests/limits, PriorityClasses, scheduling | Breaks in Module 3.1 |
| VM / template | Deployment+Pods (containers); VirtualMachine (Module 4) | Breaks in 3.1–3.2 |
| Port group / dvSwitch | OVN-Kubernetes, NAD/Multus | Breaks in networking deep dive |
| vSAN / datastore | StorageClass, PV/PVC, CSI | Breaks in storage deep dive |
| DRS / HA | Scheduling, affinity, PodDisruptionBudgets | Breaks in 3.1 |
| Change ticket / CAB | PR + merge (Module 1) | Slowest reframe |

<!-- ORG-SPECIFIC: our cluster topology, environments (non-prod/prod), and who operates what. -->

---

## 3.1 — Kubernetes internals & application layer (1–2 weeks)

**Topics**
- Images, registries, **Deployments, Services, Routes/Ingress, ConfigMaps, Secrets**
- **Namespaces/Projects, RBAC** — who can `get` what
- **Operators** — platform extends itself via CRDs (the same mechanism that adds VM support in Module 4)

**Official**
- [Pods](https://kubernetes.io/docs/concepts/workloads/pods/) (Kubernetes docs) — the atom
- [OpenShift learning (Red Hat Developer)](https://developers.redhat.com/learn/openshift)
- [An introduction to GitOps](https://www.redhat.com/en/blog/an-introduction-to-gitops) (read once so Module 5 connects)

**Verification**
- Given unseen Deployment YAML: identify image, replicas, port, env vars, volume mounts — unaided.
- Run a pod that fails (bad image tag / missing env). Diagnose with `oc get pods`, `describe`, `logs`, `get events` — no console.
- Expose an app with a Route; `curl` it; explain what the Route did and which operator manages it.

---

## 3.2 — Cluster operations (admin lens) (2–4 weeks, parallel with 3.1)

**Topics**
- **Cluster operators** — `oc get co` is the platform-health dashboard
- **MachineConfig / MachineConfigPool** — declarative node config (don't SSH to fix nodes)
- **Nodes** — cordon, drain, conditions (maintenance-mode equivalent)
- **Authentication** — OAuth, identity providers; diagnose "can't log in"
- **Security Context Constraints (SCCs)** — pod-level security model, no VMware equivalent; `restricted-v2` default is stricter than upstream K8s. Audit with `oc adm policy who-can use scc`.
- **Operator Lifecycle Manager (OLM)** — `CatalogSource`, `Subscription`, `InstallPlan`, `CSV`. A CSV stuck `Installing` is the most common failure.
- **Multi-tenancy** — namespace isolation (`ResourceQuota`, `LimitRange`) vs separate clusters; RBAC for self-service.
- **Observability** — Prometheus + Alertmanager (default); `PrometheusRule`, `AlertmanagerConfig`; LokiStack logging; `oc adm must-gather` / `oc adm inspect`.
- **Backup & DR** — `etcd` backup (control-plane recovery); OADP/Velero for workload PVs. Two models: *restore* (etcd + node rebuild) vs *rebuild* (GitOps re-apply) — neither fully replaces the other.

**Official**
- [OCP documentation](https://docs.redhat.com/en/documentation/openshift_container_platform/) — bookmark Operators, Nodes, Networking, Security
- [Managing Security Context Constraints](https://docs.redhat.com/en/documentation/openshift_container_platform/latest/html/authentication_and_authorization/managing-security-context-constraints)
- [Backing up etcd](https://docs.redhat.com/en/documentation/openshift_container_platform/latest/html/backup_and_restore/control-plane-backup-and-restore)
- [OADP](https://docs.redhat.com/en/documentation/openshift_api_data_protection/)
- [Monitoring overview](https://docs.redhat.com/en/documentation/openshift_container_platform/latest/html/monitoring/)

**Verification**
- `oc get co`; find a degraded operator; explain (via `describe`/events/logs) what's wrong.
- *(multi-node)* Cordon → confirm no eviction; drain → confirm pods moved; uncordon. On SNO: trace the procedure and name constraints.
- **SCCs:** deploy a pod requesting `hostNetwork: true`; find the SCC failure; `oc adm policy who-can use scc hostnetwork`; explain `restricted-v2` vs `privileged`.
- **OLM:** install via `Subscription`; break the channel; diagnose stuck `InstallPlan`/`CSV`; recover without deleting the operator.
- **Observability:** write a `PrometheusRule` firing when >10 pods `Pending` >5min; confirm in Alertmanager. Explain `must-gather` vs `inspect`.
- **Backup/DR:** run/trace an etcd backup; compare to a vSphere snapshot — what etcd preserves that GitOps rebuild can't, and vice versa.

<!-- ORG-SPECIFIC: our monitoring/alerting runbooks, backup schedules, and DR playbooks. -->

---

## 3.3 — Networking deep dive

- **OVN-Kubernetes** — default CNI; logical routers/switches managed via APIs, not a topology editor.
- **NetworkPolicy** — *allow* rules, not a firewall. Without any policy, all pod traffic is allowed.
- **Multus CNI** — multiple interfaces; primary via OVN, secondary via Multus (bridge/MACVLAN/IPVLAN).
- **NetworkAttachmentDefinition (NAD)** — CR defining a secondary network; VMs reference it in Module 4.
- **MetalLB** — LoadBalancer services on bare metal.
- **NMState** — declarative node networking (`NodeNetworkConfigurationPolicy`).
- **SR-IOV** — hardware virt for high-throughput/low-latency (telco/RAN).

**Verification:** Create a NAD bridge; attach a pod; confirm secondary interface; explain which component provisioned each. Write a default-deny + explicit-allow NetworkPolicy; confirm it blocks unlabeled traffic; explain why it is *not* an NSX firewall rule.

<!-- ORG-SPECIFIC: our CNI, network polices in force, and NADs available to workloads. -->

## 3.4 — Storage deep dive

- **StorageClass** — provisioner (CSI) + params; the datastore-type choice.
- **PVC / PV** — request vs provisioned; PVC persists independently of the pod.
- **CSI** — plugin API to storage back ends; swap back end = StorageClass change.
- **ODF** — Ceph-based SDS; RWX (CephFS) required for live migration (Module 4).
- **DataVolume / CDI** — how every VM disk is imported (Module 4).
- **VolumeSnapshot / VolumeSnapshotClass** — CSI point-in-time snapshots (data only, not memory).
- **RWX vs RWO** — live migration needs RWX.

**Verification:** Create PVC from default StorageClass; mount in pod; write file; delete pod; confirm persistence. Set reclaim `Retain` on another PVC; delete; find retained PV; explain next step. Explain why live migration needs RWX.

<!-- ORG-SPECIFIC: our StorageClasses, CSI drivers, backup/restore for PVs. -->


