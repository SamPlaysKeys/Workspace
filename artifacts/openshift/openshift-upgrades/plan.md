---
type: Plan
status: Draft
system: OpenShift
related_to: []
references:
  - "https://access.redhat.com/solutions/7004992"
---

# OpenShift EUS Upgrade & Fleet Migration Plan (4.18+)

> [!NOTE]
> This is an example document, useful for creating a robust plan for
> upgrading OCP clusters in the defined versions. The goal of this document
> is to use it as a blueprint when creating similar plans, essentially using 
> this as the "instructions to build the tool" that is needed.

<objective>
Create, update, or refine the foundational upgrade design document (`documents/upgrade-design.md`) into an enterprise-grade OpenShift 4.18+ EUS upgrade strategy for 150 clusters hosting OpenShift Virtualization workloads. The resulting report must minimize VM downtime using Live Migration, detail pre-flight health tools, evaluate fleet-wide rollout concurrency, and ground all recommendations in official Red Hat documentation retrieved via Context7 and official references.
</objective>

## 1. Documentation & Context Retrieval Protocol

### Primary Retrieval: Context7 (`ctx7`)
Before drafting recommendations or upgrade narratives, query live documentation using Context7:

1. **Resolve Library:**
   ```bash
   npx ctx7@latest library "OpenShift" "EUS upgrade path 4.18"
   ```
   *Primary Library ID:* `/openshift/openshift-docs`

2. **Fetch Documentation:**
   - **EUS Upgrade Channels & Paths (4.18 / 4.19 / 4.20):**
     ```bash
     npx ctx7@latest docs /openshift/openshift-docs "EUS upgrade path extended update support 4.18 4.19 4.20"
     ```
   - **OpenShift Virtualization & Live Migration during Host Maintenance:**
     ```bash
     npx ctx7@latest docs /openshift/openshift-docs "OpenShift Virtualization live migration node upgrade eviction strategy"
     ```
   - **Pre-Upgrade Diagnostics & Health Checks:**
     ```bash
     npx ctx7@latest docs /openshift/openshift-docs "pre upgrade health check tools adm upgrade recommend cluster operator status"
     ```

### Secondary Fallback: Red Hat Official Docs & Knowledgebase
If Context7 returns partial results for specific version-specific EUS nuances:
- Review Red Hat Solution Article: `https://access.redhat.com/solutions/7004992`
- Review Red Hat Product Documentation: `https://access.redhat.com/documentation/en-us/openshift_container_platform/4.18`

---

## 2. Alignment Checkpoint

Before generating the final report or modifying source design files:
- [ ] Inspect the baseline document (`documents/upgrade-design.md`).
- [ ] Present the proposed report outline and key assumptions to the user.
- [ ] **Alignment Prompt:** Ask *"Does this outline and scope match what you had in mind?"* and wait for explicit confirmation.

---

## 3. Execution Checklist & Structure

### Task 1: Baseline Review & Executive Summary
- [ ] Read `documents/upgrade-design.md` as the foundation document.
- [ ] Write an Executive Summary detailing target OCP versions (4.18, 4.19, 4.20) and key architectural objectives.
- [ ] Generate a complete Table of Contents for the final report.

### Task 2: Pre-Upgrade Diagnostics & Health Verification
- [ ] Identify Red Hat-sponsored health check tools (e.g., `oc adm upgrade recommend`, Must-Gather, Cluster Health Checks).
- [ ] Construct a pre-upgrade health checklist covering:
  - ClusterOperator status & API deprecation warnings.
  - Node capacity, storage readiness, and operator health.
  - Verification of machine config pools and node network state.

### Task 3: OpenShift Virtualization & Zero-Downtime Strategy
- [ ] Define VM eviction policies and Live Migration parameters.
- [ ] Detail host maintenance policies to ensure VM workloads migrate seamlessly prior to node reboots.
- [ ] Highlight VM live migration limits, network bandwidth requirements, and failure mitigation methods.

### Task 4: Step-by-Step EUS Upgrade Narrative
- [ ] Document the sequential EUS upgrade flow across intermediate and target versions.
- [ ] Detail control plane vs. worker node update sequences.
- [ ] Define post-upgrade verification steps and health checks.

### Task 5: Fleet Scale & Concurrency Evaluation (150 Clusters)
- [ ] Calculate estimated duration per cluster upgrade and total fleet completion time.
- [ ] Evaluate serial vs. parallel upgrade strategies for 150 clusters:
  - **Benefits:** Reduced overall maintenance window, standardized batch windows.
  - **Risks:** Blast radius, network/registry load, operational overhead.
- [ ] Define a recommended batching model (e.g., canary -> waves of N clusters).

### Task 6: References & Document Compilation
- [ ] Compile a comprehensive **References** section citing all official Red Hat URLs, solution articles, and Context7 doc links used.
- [ ] Write the completed findings report into the target documentation path.

---

## 4. Verification & Quality Gates

- [ ] **Standards Compliance:** Ensure all recommendations align with Red Hat official support standards.
- [ ] **Traceability:** Verify every technical recommendation includes a corresponding citation link in the References section.
- [ ] **No Unsubstantiated Claims:** Ensure time estimates and concurrency recommendations explicitly state underlying assumptions (e.g., bandwidth, node count per cluster).
