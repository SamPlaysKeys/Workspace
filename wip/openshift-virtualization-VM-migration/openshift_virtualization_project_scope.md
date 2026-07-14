# OpenShift Virtualization Upgrade Reduction Project Scope

## Project objective
Define and validate an enterprise approach for reducing upgrade-related downtime for OpenShift clusters that run virtual machines on OpenShift Virtualization, with particular focus on large-memory VMs whose live migrations may take long enough to affect service continuity.[web:2][page:2]

The scope centers on improving in-cluster live migration behavior during node maintenance and OpenShift upgrades rather than treating cross-cluster VM relocation as the default operating model. OpenShift Virtualization supports live migration between nodes in the same cluster, and that is the core mechanism this effort is intended to optimize.[page:1][page:2]

## Problem statement
Current rolling upgrade practices account for downtime, but that outcome is not acceptable as a long-term enterprise operating model for environments with many clusters and VM-based workloads. The key technical issue is that VM memory state must be transferred during migration, and for large-memory or high-dirty-rate workloads, that process can fail to converge quickly enough to meet near-zero-downtime goals.[page:2][web:2]

Because a virtual machine instance runs on one node at a time and migration copies execution state to the destination node, the project should focus on faster, more predictable convergence rather than architectures that assume live shared memory across nodes. This distinction is essential for setting realistic technical boundaries in the proposal.[page:2]

## In scope
The following work is in scope for the initial project phase:

- Document the current upgrade and VM migration workflow for OpenShift Virtualization-based clusters.[page:1][page:2]
- Characterize workload classes by VM memory size, dirty-page behavior, migration duration, and business criticality.[page:2]
- Evaluate native migration controls including pre-copy behavior, post-copy, auto-converge, bandwidth controls, completion timeout, progress timeout, and dedicated migration networking.[page:2][web:7]
- Define an automation pattern using Ansible and platform-native objects to coordinate migration, drain, upgrade, validation, and rollback checkpoints.[web:2][page:1]
- Produce a policy framework for deciding which VMs are eligible for standard live migration versus enhanced migration handling or operational exception paths.[page:2]
- Capture node expansion followed by evacuation and draining as a supporting upgrade tactic for increasing migration headroom during maintenance windows.[page:1][page:2]

## Out of scope
The following items are outside the initial scope unless explicitly added later:

- Re-architecting applications inside guest VMs to become cloud-native or container-native.
- Treating cross-cluster migration as the primary upgrade pattern for routine maintenance.
- Assuming a Red Hat-native external middleware product already exists that transparently compresses VM memory outside the KubeVirt and QEMU migration path.[web:2][page:2]
- Designing a shared-memory model between nodes to avoid copying memory during migration, because the documented model is transfer of VM state between source and target nodes.[page:2]
- Broad storage modernization efforts unrelated to migration convergence.

## Technical workstreams
| Workstream | Purpose | Primary outputs |
|---|---|---|
| Migration baseline | Measure current migration success, duration, dirty-rate sensitivity, and operational bottlenecks.[page:2] | Baseline metrics, workload classes, failure modes. |
| Policy tuning | Test migration parameters such as bandwidth caps, completion timeout, progress timeout, post-copy, and auto-converge.[page:2][web:7] | Candidate migration policy profiles by VM class. |
| Network path | Validate whether a dedicated migration network improves predictability and duration.[page:2] | Network design guidance, throughput findings. |
| Automation | Use Ansible and platform APIs/objects to coordinate migration and maintenance workflows.[web:2][page:1] | Runbooks, automation roles, orchestration logic. |
| Upgrade operations | Integrate migration controls into node drain and upgrade sequencing.[page:1] | End-to-end maintenance workflow. |
| Capacity strategy | Assess temporary node expansion to increase live migration headroom before draining upgraded nodes.[page:1][page:2] | Capacity model and decision criteria. |

## Proposed phases
### Phase 1: Discovery and baseline
Inventory the current environment shape, workload classes, migration frequency, and pain points. Establish baseline metrics for large-memory VM migration duration, timeout conditions, success rate, and impact on workload continuity.[page:2][web:2]

### Phase 2: Controlled tuning evaluation
Create a controlled test plan for pre-copy tuning, post-copy, auto-converge, bandwidth settings, timeouts, and dedicated migration networking. The goal is to identify which combinations improve convergence without creating unacceptable workload or resiliency trade-offs.[page:2][web:7]

### Phase 3: Automation design
Implement an orchestration layer that applies migration policy by VM class, initiates migration ahead of maintenance, tracks migration state, and controls drain and rollback gates. Ansible is a logical control mechanism because the discussion assumes access to enterprise automation tooling for coordinated execution.[web:2]

### Phase 4: Upgrade workflow integration
Embed the selected migration patterns into a repeatable node maintenance sequence: migrate eligible VMs, drain node, perform upgrade, validate service, and reintroduce node capacity. Include the optional temporary node expansion pattern where extra capacity materially improves migration success and maintenance flexibility.[page:1][page:2]

### Phase 5: Proposal and rollout plan
Produce a recommendation package describing supported workload classes, approved migration profiles, operational guardrails, fallback paths, and rollout sequencing across multiple clusters. The final deliverable should support executive review as well as technical implementation planning.[web:2][page:2]

## Key design assumptions
- OpenShift Virtualization live migration within a cluster is the primary technical mechanism for reducing upgrade interruption.[page:1][page:2]
- Large-memory VM behavior is mainly constrained by memory transfer rate versus dirty-page rate.[page:2]
- Post-copy can help migrations complete when pre-copy does not converge, but it introduces greater operational risk and must be approved by workload class.[page:2][web:8]
- Auto-converge may be acceptable when some CPU throttling is preferable to timeout or outage during maintenance.[page:2]
- A dedicated migration network may improve determinism and throughput for migration traffic.[page:2]
- Temporary node expansion is a capacity tactic to facilitate migration and draining, not a memory-sharing architecture.[page:1][page:2]

## Deliverables
- Current-state workflow and pain-point summary.
- Migration workload taxonomy for VM classes.
- Test matrix for migration tuning options.
- Recommended migration policy profiles by workload class.
- Automation and runbook design for maintenance orchestration.
- Upgrade sequencing model that includes standard node evacuation and the optional node expansion plus draining pattern.
- Final proposal document for stakeholder review.

## Success criteria
The project should be considered successful if it defines a repeatable upgrade approach that materially reduces interruption for VM-hosting OpenShift clusters, especially for large-memory workloads, and if it provides a documented operational model for when to use baseline pre-copy, enhanced tuning, post-copy, auto-converge, or temporary node expansion.[page:2][web:2][web:7]
