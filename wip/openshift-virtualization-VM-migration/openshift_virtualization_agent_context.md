# Agent Context: OpenShift Virtualization Upgrade Conversation

## Purpose
This file provides durable context for future agents working on the same project thread. It captures the technical direction, assumptions, terminology, constraints, and strategic ideas established in the conversation so later work can stay consistent with the decisions already discussed.

## Project context
The user is working on a Red Hat consulting project focused on reducing downtime during upgrades of OpenShift environments that run virtual machines through OpenShift Virtualization. The environment can be assumed to include a large number of clusters, and the current use of rolling upgrades is acknowledged but considered insufficient as the long-term answer.[web:2][page:1][page:2]

The key operational problem is that VMs consume enough RAM that migration activity can take long enough to cause outages or unacceptable service interruption. The conversation repeatedly narrowed toward a near-zero-downtime objective for upgrades, especially by improving the live migration behavior of large-memory VMs.[page:2][web:2]

## Technical baseline
OpenShift Virtualization is the virtualization layer under discussion, not oVirt as the primary platform. It is built on KubeVirt, and the conversation established that the relevant migration mechanism is KubeVirt live migration inside a cluster rather than routine cross-cluster migration as the baseline design.[page:1][page:2]

KubeVirt live migration moves a running VM instance from one node to another by transferring execution state, especially RAM, to the target node while the guest continues running until cutover. A VM runs actively on one node at a time, so the architecture is based on copying state between nodes, not on maintaining a shared live memory space across nodes.[page:2]

## Strategy direction
The preferred strategic direction is to improve in-cluster live migration enough that OpenShift node upgrades can proceed with minimal interruption. The conceptual upgrade flow is: live migrate eligible VMs off a node, drain and upgrade the node, validate, restore capacity, and repeat across the cluster.[page:1][page:2]

The strongest technical framing is to treat the challenge as a migration convergence problem. Large-memory VMs are difficult because pre-copy migration can struggle when the guest dirty-page rate is too high relative to the available network and processing capacity.[page:2]

## Native controls to consider
The conversation identified several native or near-native controls that should remain central in future planning:

- Pre-copy migration as the default baseline, because KubeVirt documents it as the standard and generally safest path for most migrations.[page:2]
- Post-copy migration for selected large-memory or high-dirty-rate VM classes when pre-copy does not converge fast enough; this should always be discussed alongside its higher operational risk.[page:2][web:8]
- Auto-converge as a way to throttle guest CPU to help pre-copy finish when dirty-page churn is the main blocker.[page:2]
- Migration bandwidth limits, completion timeout, progress timeout, and parallel migration controls as tuning levers.[page:1][page:2][web:7]
- Dedicated migration networking as a way to improve determinism and throughput for migration traffic.[page:2]

## Tooling assumptions
The conversation explicitly noted access to enterprise tooling such as Ansible, AAP, and ACM. These are positioned as orchestrators, policy enforcers, and maintainers of migration states:
- **Ansible Core (Engine-only)** acts as the local CLI-based executor to interface with OpenShift APIs and orchestrate KubeVirt migration objects, drains, and reboots.
- **Ansible Automation Platform (AAP)** extends this to a centralized, auditable orchestration layer with RBAC, secure credential storage, and GitOps-ish webhook/API integrations.
- **Advanced Cluster Management (ACM)** acts as the multi-cluster fleet manager, utilizing declarative governance policies that trigger AAP job templates to manage pre-migration and post-validation checks across multiple OpenShift Virtualization environments.

The likely role for these tools is orchestration, policy application, maintenance sequencing, and automation around native OpenShift Virtualization and KubeVirt migration controls rather than replacing the underlying migration engine.[web:2][page:2]

The conversation also explored the idea of a “drop-in layer” or middleware for memory compression. The resulting direction was that there is not a simple Red Hat ecosystem product that transparently acts as an external memory-compression sidecar for OpenShift Virtualization migrations; instead, any practical auxiliary layer would more likely be orchestration, policy control, or QEMU/KubeVirt feature enablement rather than a standalone pod that sits outside the migration engine.[web:2][page:2]

## Important clarifications from the discussion
- The user corrected the discussion away from oVirt and toward OpenShift Virtualization as the relevant platform.
- QEMU is effectively part of the OpenShift Virtualization stack underneath KubeVirt, even if it is not exposed as a separately managed product surface.[page:2]
- The guest operating system inside the VM generally matters less than the runtime behavior of the workload, especially dirty-page rate and resource intensity during migration.[page:2]
- The node expansion idea should be retained as a supporting tactic, but not as the main focus of the proposal.
- When referencing node expansion, describe it as adding temporary capacity to improve evacuation and draining, not as a model where memory is shared between nodes instead of copied.[page:1][page:2]

## Narrative constraints for future documents
Future outputs should preserve these narrative choices:

- Emphasize OpenShift Virtualization and KubeVirt, not oVirt, unless explicitly making a comparison.
- Frame the problem as one of live migration convergence for large-memory VMs.
- Treat cross-cluster migration as a secondary or more complex path, not the default upgrade method.
- Keep the target outcome as near-zero-downtime upgrades.
- Present node expansion plus draining as a supporting operational tactic.
- Avoid implying that shared memory between nodes is the mechanism for successful upgrades; the documented mechanism is migration of VM state from source node to target node.[page:2]

## Reusable summary
The project is shaping into an enterprise proposal that uses OpenShift Virtualization live migration as the foundation for low-downtime node upgrades, then layers migration policy tuning, dedicated migration networking, and automation on top to improve behavior for large-memory VMs. The most likely proposal pattern is to classify workloads, assign migration profiles, orchestrate migration before node drains, and use temporary capacity expansion only where additional headroom is needed to support the maintenance cycle.[page:1][page:2][web:2]
