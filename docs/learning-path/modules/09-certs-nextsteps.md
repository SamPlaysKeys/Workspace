# Module 9 — Certifications & Next Steps

**Audience:** Engineers planning formal credentials alongside the learning path.

**Outcomes:** Map each module to relevant public certifications, choose a sensible sequence, and identify deeper specializations.

---

## Checklist — choose your track

- [ ] Pick a **vendor-neutral track** (portable across employers) and/or a **Red Hat track** (matches our OCP stack)
- [ ] **GitHub Foundations** (after Module 1) — GitHub/PSI, $99, 75Q, no prereq
- [ ] **Linux base (Red Hat):** RH124 → RH134 → **RHCSA (EX200)**
- [ ] **Kubernetes (CNCF, vendor-neutral):** **KCNA** (entry) → **CKAD** or **CKA** (66%, $445) → **CKS** (requires CKA)
- [ ] **OpenShift (Red Hat):** DO180 → DO280 → **EX280**; then DO317 (Virt + EX316 included), DO432 → **EX432** (fleet)
- [ ] Book one exam with a target date; the rest follow the module sequence below
- [ ] <!-- ORG-SPECIFIC: our exam voucher process, approved providers, required internal certs -->

## Certification map

| Certification | Track | After which module(s) | Notes |
|---------------|-------|----------------------|-------|
| **GitHub Foundations** | Vendor-neutral | Module 1 | Git + GitHub fundamentals; GitHub/PSI, $99, 75Q, no prereq; study via Microsoft Learn *GitHub Fundamentals* path |
| **RHCSA (EX200)** | Red Hat / Linux | Module 0–2 (Linux base) | RH124 → RH134 → EX200; the Linux foundation OCP assumes |
| **KCNA — Kubernetes & Cloud Native Associate** | CNCF (vendor-neutral) | Module 3 (entry) | Multiple-choice entry credential; good onboarding milestone |
| **CKAD — Certified Kubernetes Application Developer** | CNCF (vendor-neutral) | Module 3 (app layer) | Performance-based, 66%, $445; for those deploying apps to K8s |
| **CKA — Certified Kubernetes Administrator** | CNCF (vendor-neutral) | Module 3 depth | Performance-based, 66%, $445; the benchmark admin cert |
| **CKS — Certified Kubernetes Security Specialist** | CNCF (vendor-neutral) | after CKA | Requires a valid CKA; advanced security |
| **EX280 — Red Hat Certified OpenShift Administrator** | Red Hat | Module 3 depth | Core cluster admin; 3hr perf-based, 210/300; prep with DO180 → DO280 |
| **EX280 — Red Hat Certified OpenShift Administrator** | Red Hat | Module 3 depth | Core cluster admin; 3hr perf-based, 210/300; prep with DO180 → DO280 |
| **EX316 — Red Hat Certified Specialist in OpenShift Virtualization** | Red Hat | Module 4 depth | Included in DO317 bundle; covers VM management with KubeVirt |
| **EX432 — Red Hat Certified Specialist in OpenShift Advanced Cluster Management** | Red Hat | Module 6 depth | RHACM, policy, fleet governance; prep with DO432; replaces retired EX480. Red Hat lists DO380 (OpenShift Admin III) as a recommended prereq — optional but useful for deeper cluster-scaling context |
| **DO180 / DO280 / DO317 / DO432** | Red Hat (courses) | Modules 2 / 3 / 4 / 6 | Formal Red Hat courses aligned to each module; DO317 bundles EX316 exam |

<!-- ORG-SPECIFIC: our exam voucher process, approved providers, and any required internal certs. -->

## Suggested sequencing

1. **GitHub Foundations** (Module 1) — early, low-cost, universally useful.
2. **RHCSA (EX200)** — if Linux is new to you; the base OCP assumes (RH124 → RH134 → EX200).
3. **KCNA → CKA/CKAD** after Module 3 — vendor-neutral, portable Kubernetes proof.
4. **EX280** after Module 3 — validates core OCP admin on our stack.
5. **EX432** after Module 6 — validates fleet governance for platform roles. Red Hat recommends DO280 → DO380 (optional) → DO432 as the prep path; DO380 covers cluster scaling and is useful but not required to attempt the exam.
6. Role-specific: DO317 (Virt, includes EX316) for virtualization focus; **CKS** (after CKA) for security; ZTP/specialist for edge fleets.

## Beyond this path

- Contribute to internal runbooks; mentor the next cohort through Modules 0–1.
- Deepen weakest verification checks — they mark real operational gaps.
- Track upstream changes (course renumbers, ZTP tooling, ACM policy API) and update this path.


