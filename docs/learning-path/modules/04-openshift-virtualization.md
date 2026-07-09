# Module 4 — OpenShift Virtualization

**Audience:** Platform engineers who need to run and migrate VMs on Kubernetes/OpenShift.

**Outcomes:** Create, start, stop, and migrate VMs; connect storage/networking; understand KubeVirt primitives (`VirtualMachine`, `VirtualMachineInstance`, `DataVolume`) as Kubernetes resources — because that is what they are; perform live migration and plan bulk migration from vSphere (MTV).

**Prerequisite:** Module 3 substantially complete. A `VirtualMachineInstance` *is a Pod* — you cannot operate it without understanding Pods, scheduling, networking, and storage.

---

## Checklist — work through in order

- [ ] Confirm Module 3 is substantially complete (a VMI *is* a Pod)
- [ ] Read the mental model + skim [About OpenShift Virtualization](https://docs.redhat.com/en/documentation/openshift_container_platform/latest/html/virtualization/about-virt)
- [ ] Browse the [Red Hat OpenShift Virtualization learning hub](https://www.redhat.com/en/technologies/cloud-computing/openshift/virtualization/learn) and [kubevirt.io](https://kubevirt.io/) for the upstream project
- [ ] Create/start/stop a VM; find its backing VMI Pod
- [ ] Practice live migration (needs multi-node + RWX) and walk one MTV migration plan
- [ ] **Formal course:** [DO316](https://www.redhat.com/en/services/training/do316-managing-virtual-machines-red-hat-openshift-virtualization) (or DO156 Admin I / DO256 Admin II)
- [ ] Pass the scenario-based Verification at the bottom

<!-- ORG-SPECIFIC: Virt-enabled clusters, RWX storage classes, migration source environment -->

---

## Mental model

OpenShift Virtualization runs VMs *inside* Kubernetes via KubeVirt. The hypervisor is a process inside a Pod. The scheduler does not know or care it's a VM. This is why Modules 2–3 come first.

## Topics

- **KubeVirt primitives** — `VirtualMachine` (desired-state), `VirtualMachineInstance` (running Pod with hypervisor), `DataVolume` (managed PVC + import pipeline)
- **VM networking** — secondary networks via NAD; pod overlay (OVN) vs bridge-attached VLANs
- **VM storage** — DataVolume import (HTTP/registry/PVC clone); RWX requirement for live migration; VolumeSnapshot for disk backups
- **Live migration** — mechanism, objects (`VirtualMachineInstanceMigration`), shared-storage + node-scheduling requirements
- **Migration Toolkit for Virtualization (MTV)** — bulk vSphere import: source inventory, network maps, storage maps, cutover
- **Performance tuning (read-ahead)** — CPU pinning/NUMA (`dedicatedCpuPlacement`), HugePages, SR-IOV, real-time kernel, Node Tuning Operator

## Official

- [About OpenShift Virtualization](https://docs.redhat.com/en/documentation/openshift_container_platform/latest/html/virtualization/about-virt)
- [Migrating VMs from vSphere (MTV)](https://docs.redhat.com/en/documentation/openshift_container_platform/latest/html/migration_toolkit_for_virtualization/)
- [VMs as code with GitOps + Virt](https://cloud.redhat.com/blog/virtual-machines-as-code-with-openshift-gitops-and-openshift-virtualization)

**Formal courses:** [DO316 — Managing VMs with OpenShift Virtualization](https://www.redhat.com/en/services/training/do316-managing-virtual-machines-red-hat-openshift-virtualization); DO156/DO256 Administration I/II.

<!-- ORG-SPECIFIC: our Virt-enabled clusters, available storage classes for RWX, and migration source environment. -->

## Verification (scenario-based)

1. Create a VM from template/YAML; console/SSH in; find the backing VMI Pod via `oc get pods`; describe it; confirm it's what Kubernetes schedules.
2. **Live migration** *(multi-node + RWX required; not on SNO)*: migrate between workers; explain which K8s mechanisms determined placement. *SNO fallback:* trace every object (`VirtualMachineInstanceMigration`, source/target VMI pods, shared RWX PVC, QEMU channel); explain why SNO can't (no valid target node) and the minimum topology.
3. Walk one MTV planning chapter: source inventory, network/storage maps, cutover — even if you migrate only one small VM.


