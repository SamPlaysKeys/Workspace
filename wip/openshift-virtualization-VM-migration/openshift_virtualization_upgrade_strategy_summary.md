# OpenShift Virtualization Upgrade Strategy Summary

## Overview
This discussion focused on reducing upgrade-related downtime for large OpenShift environments that run virtual machines through OpenShift Virtualization. The central issue is that VM memory footprints are large enough that moving VMs during maintenance or upgrade windows can take long enough to create outages, even when existing operations already use rolling upgrade patterns.[web:2][page:1][page:2]

The main direction explored was not cross-cluster VM relocation as the primary mechanism, but improving in-cluster live migration so VMs can be moved between nodes while cluster maintenance proceeds with minimal or near-zero interruption. OpenShift Virtualization supports live migration for running VM instances between nodes in the same cluster while the guest remains accessible, which makes node-by-node maintenance the more natural baseline mechanism.[page:1][page:2]

## Core migration model
OpenShift Virtualization is built on KubeVirt, and KubeVirt live migration works by moving a running virtual machine instance from one compute node to another while the guest workload continues to run. In the standard live-migration path, the source VM transfers its state, primarily RAM, to a target node, and KubeVirt manages that process through migration objects and cluster-wide migration controls.[page:2][page:1]

This matters architecturally because a VM is actively running on one node at a time, and the migration process is fundamentally about copying execution state to a new node rather than sharing a single in-memory instance across nodes. For upgrade planning, that means the design target is to make state transfer converge faster and more predictably, not to assume shared live memory across nodes.[page:2]

## Main strategy under discussion
The most promising strategy discussed was to stay within the OpenShift Virtualization stack and improve migration behavior with memory-management and migration-tuning controls layered on top of the platform. KubeVirt documents pre-copy as the default migration strategy, where the guest keeps running on the source while memory pages are copied to the target until enough of the state has converged for cutover.[page:2]

The challenge for large-memory VMs is the guest dirty rate: if the VM keeps modifying memory faster than memory can be copied, migration may stall, time out, or take too long to meet enterprise upgrade objectives. KubeVirt explicitly notes that high dirty-rate workloads and insufficient resources such as network bandwidth or CPU can prevent migration from converging in a reasonable amount of time.[page:2]

## Memory and compression-adjacent levers
A practical proposal can frame the problem as reducing the amount of memory that must be resent, reducing the time needed to send it, or allowing migration to complete when pre-copy alone does not converge. In OpenShift Virtualization and KubeVirt, the main native levers are migration bandwidth controls, completion timeout, progress timeout, post-copy enablement, auto-converge, and the option to place migration traffic on a dedicated network.[page:1][page:2][web:7]

Post-copy is especially relevant for large-memory or busy VMs because KubeVirt states that it allows even very busy VMIs to migrate successfully by switching away from pure pre-copy once acceptable completion time is exceeded. The trade-off is that post-copy carries more operational risk: if failure occurs during the post-copy phase, VM state may not be recoverable, and the workload can also see a slower warm-up because memory is fetched on demand from the source.[page:2][web:8]

Auto-converge is another useful lever because it helps pre-copy finish by throttling the guest CPU when high dirty-page rates would otherwise keep the migration from converging. That makes it a strong candidate for enterprise testing where the objective is not perfect transparency at all costs, but predictable completion of maintenance workflows with an acceptable, measured performance impact.[page:2]

Although there is not a simple separate “compression middleware” product in the Red Hat stack that can be dropped in as an external pod and transparently compress memory outside the migration path, the proposal can still describe an auxiliary control layer around native migration features. In practice, that layer would likely be orchestration and policy automation rather than a standalone memory-sharing service, because the migration engine remains inside the KubeVirt and QEMU stack.[page:2][web:2]

## Proposed enterprise approach
A robust proposal would position Ansible and platform automation as the control plane around native OpenShift Virtualization migration features. That control plane could classify VMs by memory size, dirty-page behavior, and business criticality; apply different migration policies; stage migrations before node drains; and monitor migration progress and fallback conditions using the existing migration objects and status reporting exposed by the platform.[page:1][page:2]

A sensible phased strategy could look like this:

- Baseline on pre-copy for most workloads, because KubeVirt describes it as the safest and fastest option for most cases.[page:2]
- Identify large-memory or high-dirty-rate VMs that regularly fail to converge, then test post-copy only for approved workload classes where the resilience trade-off is acceptable.[page:2][web:8]
- Enable auto-converge for VM classes where CPU throttling is preferable to repeated migration timeout or prolonged maintenance windows.[page:2]
- Move migration traffic to a dedicated migration network where possible, because KubeVirt documents this as a way to gain more determinism, control, and bandwidth for migrations.[page:2]
- Tune cluster-wide limits such as bandwidth per migration, parallel migrations per cluster, parallel outbound migrations per node, completion timeout per GiB, and progress timeout so that heavy migrations do not overwhelm the environment.[page:1][page:2][web:7]

## Upgrade workflow implications
For upgrades, the operational pattern remains node-level evacuation followed by maintenance, not cross-cluster VM motion as the default answer. OpenShift Virtualization supports the `LiveMigrate` eviction strategy so that when a node is drained or put into maintenance, eligible VM workloads can be live migrated to another node rather than interrupted.[page:1]

That means a near-zero-downtime upgrade pattern can be framed as: migrate eligible VMs within the cluster, drain and upgrade the selected node, return capacity to service, and repeat. The success of that pattern depends on making memory transfer predictable enough for large-memory VMs, which is why migration tuning, post-copy policy, auto-converge, and dedicated migration networking are the most relevant technical levers in this proposal.[page:1][page:2]

A secondary expansion strategy also came up and should be captured as a supporting option: temporarily add nodes, evacuate workloads onto the added capacity, then drain and upgrade the original nodes in sequence. That approach can make upgrades easier by increasing headroom for live migration and maintenance, but it should be presented as a capacity and orchestration tactic rather than as a design that requires memory to be shared between nodes instead of copied.[page:1][page:2]

## Key recommendation
The strongest proposal direction is to treat large-memory live migration in OpenShift Virtualization as a convergence problem and solve it with layered controls already aligned to the platform: migration policy tuning, workload segmentation, post-copy where justified, auto-converge where tolerated, and dedicated migration networking where available. Ansible and related automation can provide the enterprise-grade coordination needed to turn those native features into a repeatable upgrade workflow with materially lower downtime risk.[page:2][web:2][web:7]
