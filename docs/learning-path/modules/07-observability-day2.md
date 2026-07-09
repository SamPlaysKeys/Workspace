# Module 7 — Observability & Day-2 Operations

**Audience:** Engineers responsible for keeping workloads and clusters healthy, observable, and recoverable.

**Outcomes:** Monitor with Prometheus/Alertmanager; aggregate logs with Loki; define SLO-style alerts; back up and recover workloads (OADP) and the control plane (etcd); reason about the two recovery models.

**Builds on:** Module 3.2 (observability + backup/DR sections).

---

## Checklist — work through in order

- [ ] Read the Day-2 mental model below (*detect and recover*, not *click and hope*)
- [ ] **Metrics** — [Prometheus docs](https://prometheus.io/docs/introduction/overview/) + [OCP Monitoring](https://docs.redhat.com/en/documentation/openshift_container_platform/latest/html/monitoring/); enable user-workload monitoring, write a `PrometheusRule`
- [ ] **Dashboards & alerting** — [Grafana docs](https://grafana.com/docs/); wire Alertmanager routes
- [ ] **Logs** — deploy and query **LokiStack** (current logging backend)
- [ ] **Diagnostics** — practice `oc adm must-gather` vs `oc adm inspect`
- [ ] **Backup & DR** — [OADP/Velero](https://docs.redhat.com/en/documentation/openshift_api_data_protection/) for workloads + **etcd backup** for the control plane; understand the two recovery models (restore vs rebuild)
- [ ] Complete the scenario-based verification at the bottom
- [ ] <!-- ORG-SPECIFIC: our alerting routes, log retention, backup schedules, restore drills -->

## Mental model

Day-2 operations is *detect and recover*, not *click and hope*. Observability tells you what's wrong; backup/DR tells you how to get back. Both are declarative and version-controllable.

## Topics

- **Metrics** — Prometheus + Alertmanager (installed by default); **user-workload monitoring** is opt-in (`enableUserWorkload: true` in cluster-monitoring ConfigMap). Key objects: `PrometheusRule`, `AlertmanagerConfig`.
- **Logs** — LokiStack is the current logging backend (replaced EFK/Elasticsearch).
- **Diagnostics** — `oc adm must-gather` (full snapshot for support/post-incident) vs `oc adm inspect` (targeted resource inspection).
- **Fleet observability** — ACM Observability aggregates hub-level metrics across managed clusters.
- **Backup & DR** — `etcd` backup = control-plane recovery (certs, cluster ID, custom CRs). **OADP/Velero** backs up workload namespaces, PVs, metadata to object storage; Virt VMs need the KubeVirt Velero plugin.
- **Two recovery models:** *restore* (etcd backup + node rebuild — recovers cluster state) vs *rebuild* (GitOps re-apply — recovers workloads but not cluster state). Neither replaces the other.

## Official

- [Monitoring overview](https://docs.redhat.com/en/documentation/openshift_container_platform/latest/html/monitoring/)
- [OADP](https://docs.redhat.com/en/documentation/openshift_api_data_protection/)
- [Backing up etcd](https://docs.redhat.com/en/documentation/openshift_container_platform/latest/html/backup_and_restore/control-plane-backup-and-restore)

<!-- ORG-SPECIFIC: our alerting routes (PagerDuty/Slack), log retention, backup schedules, and restore drills. -->

## Verification (scenario-based)

1. Write a `PrometheusRule` firing when any namespace has >10 pods `Pending` >5min; confirm in Alertmanager. (Prereq: enable user-workload monitoring.)
2. Explain what `oc adm must-gather` collects and when to use it vs `oc adm inspect`.
3. Run/trace an etcd backup; describe contents, storage location, restore procedure. Compare to a vSphere snapshot: what etcd preserves that GitOps rebuild can't, and vice versa.
4. Explain the OADP backup flow for a Virt VM and why the KubeVirt Velero plugin is required for consistent VM backup.


