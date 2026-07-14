# OpenShift Learning Curriculum

## Purpose

This curriculum provides structured, role-based learning paths for engineers and support staff working with the OpenShift / RHOSV platform. Each path draws from a shared set of modules; the modules differ in depth and scope by role.

**Philosophy:** Learn the *model* before the *commands*. Most failures come from a wrong mental model, not a missing command. Every module front-loads the mental shift, then the mechanics. Treat the lab as non-negotiable — concepts should map to something you can inspect in a console or from a CLI.

> Modules are maintained in [learning-path/modules/](learning-path/modules/). Each module page is self-contained: audience, outcomes, topics, curated resources, scenario-based verification, and `<!-- ORG-SPECIFIC -->` slots for internal details.

---

## Role Paths

### Platform / Infrastructure Engineers

**Audience:** Server engineers, platform engineers, and infrastructure engineers who build, operate, and extend the OpenShift platform.

**Outcome:** Full platform-engineering foundation — from Linux and Git fundamentals through cluster operations, virtualization, GitOps-driven delivery, fleet governance, observability, and (for specialists) zero-touch provisioning.

| # | Module | Required / Optional |
|---|--------|---------------------|
| 0 | [Prerequisites & How to Use](learning-path/modules/00-prerequisites.md) | Required |
| 1 | [Git, GitHub & GitLab Best Practices](learning-path/modules/01-git.md) | Required |
| 2 | [Containers & Registries](learning-path/modules/02-containers.md) | Required |
| 3 | [Kubernetes & OpenShift Core](learning-path/modules/03-k8s-openshift-core.md) | Required |
| 4 | [OpenShift Virtualization](learning-path/modules/04-openshift-virtualization.md) | Required |
| 5 | [CI/CD & GitOps](learning-path/modules/05-cicd-gitops.md) | Required |
| 6 | [Fleet Management with ACM](learning-path/modules/06-fleet-acm.md) | Required |
| 7 | [Observability & Day-2 Operations](learning-path/modules/07-observability-day2.md) | Required |
| 8 | [Zero Touch Provisioning](learning-path/modules/08-ztp.md) | Optional — specialist track (50+ cluster / edge fleets) |
| 9 | [Certifications & Next Steps](learning-path/modules/09-certs-nextsteps.md) | Required |

**Suggested sequence:** Complete modules in order. Module 3 is the largest — work through its sub-sections (app layer → cluster ops → networking → storage) before moving to Module 4.

**Expected outcomes:**

- Perform essential Linux administration tasks and live on the CLI
- Work confidently with Git, GitHub, and GitLab-based collaboration workflows
- Build, run, and reason about container images and registries
- Deploy, diagnose, and operate workloads and cluster-level concerns on Kubernetes/OpenShift
- Run and migrate VMs on Kubernetes via OpenShift Virtualization (KubeVirt)
- Build CI/CD pipelines and drive cluster state declaratively with Argo CD (GitOps)
- Govern a multi-cluster fleet with RHACM policies and compliance
- Monitor, alert, back up, and recover workloads and clusters
- (Specialist) Automate full cluster lifecycle via the GitOps ZTP pipeline

---

### Support Engineers

**Audience:** Support staff who diagnose, triage, and escalate issues on the OpenShift platform. Operational and diagnostic depth; does not include pipeline engineering, GitOps authoring, fleet governance, or ZTP.

**Outcome:** Solid operational foundation — understand the platform model, navigate the CLI and console confidently, diagnose workload and cluster issues, and apply observability tooling to identify and escalate problems.

| # | Module | Required / Optional |
|---|--------|---------------------|
| 0 | [Prerequisites & How to Use](learning-path/modules/00-prerequisites.md) | Required |
| 1 | [Git, GitHub & GitLab Best Practices](learning-path/modules/01-git.md) | Required |
| 2 | [Containers & Registries](learning-path/modules/02-containers.md) | Required |
| 3 | [Kubernetes & OpenShift Core](learning-path/modules/03-k8s-openshift-core.md) | Required |
| 4 | [OpenShift Virtualization](learning-path/modules/04-openshift-virtualization.md) | Required |
| 7 | [Observability & Day-2 Operations](learning-path/modules/07-observability-day2.md) | Required |
| 9 | [Certifications & Next Steps](learning-path/modules/09-certs-nextsteps.md) | Required |

> Modules 5 (CI/CD & GitOps), 6 (Fleet Management), and 8 (ZTP) are not included in the support path. Support staff should be *consumers* of the GitOps/fleet model — understanding that Git is the source of truth and the console is read/observe — but are not expected to author pipelines or ACM policies.

**Suggested sequence:** Complete modules in order. Skip from Module 4 directly to Module 7 — observability and diagnostics are the primary day-to-day toolset for support work.

**Expected outcomes:**

- Navigate the OpenShift console and CLI to inspect cluster and workload state
- Understand the container and Kubernetes model well enough to reason about failures
- Diagnose and triage VM issues on OpenShift Virtualization
- Use `oc adm must-gather`, `oc adm inspect`, Prometheus, and LokiStack to gather diagnostic data
- Understand backup/DR models (OADP/Velero, etcd) to support restore operations
- Escalate effectively with structured, evidence-based incident summaries

---

## Module Library

All modules live in [learning-path/modules/](learning-path/modules/). Each is self-contained and can be read independently, though the paths above define the recommended sequence.

| # | Module | Short description |
|---|--------|-------------------|
| 0 | [Prerequisites & How to Use](learning-path/modules/00-prerequisites.md) | Lab setup, verification philosophy, beginner's mind |
| 1 | [Git, GitHub & GitLab Best Practices](learning-path/modules/01-git.md) | Git object model, daily workflow, GitHub Foundations cert |
| 2 | [Containers & Registries](learning-path/modules/02-containers.md) | Images, layers, tags vs digests, registry auth |
| 3 | [Kubernetes & OpenShift Core](learning-path/modules/03-k8s-openshift-core.md) | App layer, cluster ops, networking, storage |
| 4 | [OpenShift Virtualization](learning-path/modules/04-openshift-virtualization.md) | KubeVirt VMs, live migration, MTV from vSphere |
| 5 | [CI/CD & GitOps](learning-path/modules/05-cicd-gitops.md) | Pipelines, Argo CD, GitOps repo structure, secrets |
| 6 | [Fleet Management with ACM](learning-path/modules/06-fleet-acm.md) | RHACM policies, compliance, ACM + GitOps hybrid |
| 7 | [Observability & Day-2 Operations](learning-path/modules/07-observability-day2.md) | Prometheus, Loki, OADP, etcd backup/DR |
| 8 | [Zero Touch Provisioning](learning-path/modules/08-ztp.md) | SiteConfig, PolicyGenTemplate, TALM — specialist track |
| 9 | [Certifications & Next Steps](learning-path/modules/09-certs-nextsteps.md) | RHCSA, KCNA, CKA, CKAD, EX280, EX480 map |
