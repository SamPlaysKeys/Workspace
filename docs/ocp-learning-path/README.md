# DevOps Learning Path — Index

> **Purpose:** A self-paced curriculum for (1) upskilling current engineers and (2) onboarding new hires into our DevOps / platform practices. Work through modules in order; each builds on the previous. Every module ends with scenario-based verification — if you cannot do the check without notes, the module is not complete.
>
> **How to use:** Start at Module 0. Each module page is self-contained: audience, outcomes, topics, curated resources, verification, and `<!-- ORG-SPECIFIC -->` slots where you fill in our internal details (clusters, tooling, runbooks, contacts).
>
> **Philosophy:** Learn the *model* before the *commands*. Most failures come from a wrong mental model, not a missing command. Each module front-loads the mental shift, then the mechanics.

## Modules

| # | Module | What you'll be able to do |
|---|--------|---------------------------|
| 0 | [Prerequisites & How to Use](00-prerequisites.md) | Set up a lab, understand the verification approach, adopt a beginner's mind |
| 1 | [Git, GitHub & GitLab Best Practices](01-git.md) | Use Git as daily change management; pass GitHub Foundations; operate in GitHub + GitLab |
| 2 | [Containers & Registries](02-containers.md) | Build, run, and store container images; reason about layers and registries |
| 3 | [Kubernetes & OpenShift Core](03-k8s-openshift-core.md) | Deploy & diagnose apps on K8s/OCP; operate cluster-level concerns (SCC, OLM, networking, storage) |
| 4 | [OpenShift Virtualization](04-openshift-virtualization.md) | Run & migrate VMs on Kubernetes via KubeVirt; live migration; MTV |
| 5 | [CI/CD & GitOps](05-cicd-gitops.md) | Build pipelines; drive cluster state with Argo CD; structure a GitOps repo; handle secrets |
| 6 | [Fleet Management with ACM](06-fleet-acm.md) | Govern many clusters declaratively with policies, compliance, and hybrid ACM+GitOps |
| 7 | [Observability & Day-2 Operations](07-observability-day2.md) | Monitor, log, alert, back up, and recover workloads and clusters |
| 8 | [Zero Touch Provisioning (Specialist)](08-ztp.md) | Automate full cluster lifecycle (Day 0/1/2) via the GitOps ZTP pipeline |
| 9 | [Certifications & Next Steps](09-certs-nextsteps.md) | Map modules to public certifications; plan your path |

## Conventions used in this path

- **Verification** = scenario-based, not definition-recall. Each module has checks to self-assess.
- **`<!-- ORG-SPECIFIC: ... -->`** markers flag where internal context must be inserted before publishing.
- **Public resources** are linked throughout; supplement with internal runbooks where noted.

---

*Maintained as a living document. Add internal runbook links and org standards under each module as they develop.*
