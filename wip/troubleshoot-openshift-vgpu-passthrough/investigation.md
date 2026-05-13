# OpenShift: vGPU passthrough

**Status:** investigating

**Environment:** _TBD — capture OpenShift version, bare metal vs nested virt, hypervisor (if any), GPU vendor/model, NVIDIA vGPU software build, and whether you use the NVIDIA GPU Operator, OpenShift Virtualization (CNV), or both._

---

## Program scope (single delivery)

This is **one program**, not two side quests: **VMware workload migration** (MTV + VDDK and related provider/storage/network work) and **OpenShift Virtualization GPU/vGPU** setup are being executed **together** right now.

- **Shared surface:** Same cluster, same **OpenShift Virtualization / HyperConverged** lifecycle, possible contention on **MachineConfig** reboots, storage classes, and who edits **HC** (mediated devices / feature gates touch the same CR MTV relies on).
- **Product sequencing:** Migrated VM specs from VMware **do not** automatically include OpenShift-side GPU devices; combined delivery still means deciding **when** migrated workloads get `spec.domain.devices.gpus` and guest drivers relative to cutover (lift-and-shift first vs GPU-ready destination from day one).

---

## Primary reference (workflow)

Progressive documentation tracks our path against NVIDIA’s guide for **GPU Operator + OpenShift Virtualization**:

[NVIDIA GPU Operator with OpenShift Virtualization](https://docs.nvidia.com/datacenter/cloud-native/openshift/latest/openshift-virtualization.html)

Red Hat procedures linked from that page (PCI passthrough / vGPU in HyperConverged) are the **OpenShift Virtualization–native** alternative when not using the full Operator path for device configuration.

---

## Goals (emergent)

_Build this list as decisions land. Strike through when done._

- [ ] Confirm target: **vGPU for KubeVirt VMs** (mediated devices + sandbox device plugin) vs **full GPU passthrough** (`vm-vgpu` vs `vm-passthrough` node label and different HyperConverged / ClusterPolicy flags).
- [ ] **Combined track:** VMware → OpenShift **MTV migration** (VDDK, providers, plans, cutover) **with** host/GPU enablement — agree ordering (e.g. generic migration vs GPU-attached destination VMs) and ownership of **HyperConverged** edits.
- [ ] _Add further goals per phase (IOMMU, image build, Operator, HC CR, first VM)._

---

## Workflow checklist (MTV + CNV — migration thread)

_Index for the VMware side of the same program; fill **Done / notes** as the team progresses._

| Step | Summary | Done / notes |
|------|---------|--------------|
| M1 | VDDK available to MTV (entitlement, image/secret, operator expectations) | **Issue:** image in private Quay tagged **`latest`** only (no immutable version tag). **Plan:** pull VDDK from [VMware Virtual Disk Development Kit (VDDK)](https://developer.broadcom.com/sdks/vmware-virtual-disk-development-kit-vddk/latest), rebuild/repush. **Decision:** Broadcom offers **9.0.0.0** and **9.0.1.0**; team standardizes on **`9.0.0.0`** as the Quay tag (not `9.0.1.0`). Clusters whose MTV config still references **`:latest`** keep working unchanged; they do not automatically pick up **`9.0.0.0`** until the CR is updated. **Caveat:** do not delete or repoint **`latest`** on Quay until every cluster either references **`9.0.0.0`** or you accept breakage for stragglers on **`latest`**. |
| M2 | VMware provider + storage/network mappings; migration plans | |
| M3 | Cutover / validation criteria for migrated workloads | |
| M4 | **GPU follow-on:** post-migration VMI patches for `devices.gpus` + guest driver/licensing if required | |

---

## Workflow checklist (NVIDIA doc — vGPU path)

_Use as a living index; note subsection + date in **Investigation** when each step is attempted or completed._

| Step | Doc section (summary) | Done / notes |
|------|------------------------|--------------|
| 1 | Prerequisites: CNV installed, `virtctl`, `disableMDevConfiguration` on `HyperConverged`, vGPU BIOS/prereqs (e.g. SR-IOV for Ampere+) | |
| 2 | IOMMU: `MachineConfig` kernel args (`intel_iommu=on` / `amd_iommu=on`) | |
| 3 | Label workers: `nvidia.com/gpu.workload.config=vm-vgpu` (mutually exclusive with `container` / `vm-passthrough` on same node) | |
| 4 | Build/push **vGPU Manager** image (`vgpu-manager/...`, `OS_TAG` matches RHCOS minor) | |
| 5 | Install GPU Operator + `ClusterPolicy`: `sandboxWorkloads.enabled`, sandbox device plugin, vGPU Manager + vGPU Device Manager | **Next (runbook):** edit/apply **`ClusterPolicy`** (`nvidia.com/v1`) — see Investigation **NVIDIA ClusterPolicy**. |
| 6 | Optional: `nvidia.com/vgpu.config` node label for device-manager profile | |
| 7 | **HyperConverged**: `permittedHostDevices.mediatedDevices` with `externalResourceProvider: true`, `mdevNameSelector` / `resourceName` matching allocatable `nvidia.com/...` | |
| 8 | VM: `spec.domain.devices.gpus` with `deviceName` matching permitted resource | |

**Doc constraints worth re-checking:** one GPU workload mode per node; GPU VM nodes assumed **bare metal**; **MIG-backed vGPUs not supported**; guest vGPU driver inside VM is **not** automated by the Operator.

---

## Symptoms

- _Setup in progress for vGPU passthrough on OpenShift._
- _Add concrete symptoms as they appear: scheduling failures, device plugin errors, VM start failures, `nvidia-smi` inside guest, MIG/vGPU license errors, etc._

---

## Investigation

### 2026-05-13 — Session start

**Hypothesis:** _None yet — scoping the stack (passthrough vs mediated devices, single vs multi-tenant vGPU) will drive the first checks._

**Tried:** Opened structured troubleshoot session per `workstyle/working_style.md`.

**Result:** Working log established; next steps are environment confirmation and first failing signal (logs, events, or desired vs actual cluster state).

### 2026-05-13 — Reference workflow locked

**Hypothesis:** Treating the NVIDIA OpenShift Virtualization chapter as the canonical ordered workflow keeps progressive docs aligned with vendor-tested sequencing (prereqs → IOMMU → labels → vGPU image → Operator → HC → VM).

**Tried:** Fetched and summarized [NVIDIA GPU Operator with OpenShift Virtualization](https://docs.nvidia.com/datacenter/cloud-native/openshift/latest/openshift-virtualization.html); added reference, emergent **Goals**, and **Workflow checklist** to this file.

**Result:** Session is anchored for incremental goal-setting and step-by-step notes; checklist rows get filled as we execute each phase.

### 2026-05-13 — Program framing: migration + GPU combined

**Hypothesis:** Running MTV/VDDK and NVIDIA vGPU prep **as one delivery** increases need for an explicit order: HC changes (e.g. `disableMDevConfiguration`, `permittedHostDevices`) and node reboots should be coordinated with migration windows so the team does not fight the same CR or nodes.

**Tried:** User clarified this is a **VMware workload migration** and the two tracks are **combined right now**; updated **Program scope**, **Goals**, and added **Workflow checklist (MTV + CNV)**.

**Result:** Log matches single-program reality; use both checklists as a joint runbook (M* for migration, numbered steps for GPU host/VM).

### 2026-05-13 — VDDK image tagging (Quay)

**Hypothesis:** MTV and ops need a **pinned** VDDK init image reference (`registry/org/repo:<version>` or digest). A lone **`latest`** tag is ambiguous (unknown SDK build, bad rollback story, pull policies may not pick up “same” `latest` twice).

**Tried:** User reported collaborator pushed VDDK image to private Quay with tag **`latest`** instead of an explicit version; team will re-source from Broadcom VDDK SDK page, repush with a proper tag, and remove **`latest`**.

**Result:** M1 notes updated. **Follow-through:** after the new tag exists, point **ForkliftController** (or whatever MTV UI/backend field holds the global VDDK init image) at the versioned tag; confirm pull secret still covers the repo; restart or roll MTV controller-related workloads if the operator caches image spec; then remove **`latest`** in Quay (UI *Tags* → delete, or `skopeo delete docker://…/repo:latest`) so nobody re-accidentally depends on it.

### 2026-05-13 — VDDK version pin (9.0.0.0 vs 9.0.1.0)

**Hypothesis:** Pushing **`9.0.0.0`** as an additional tag does not disturb clusters still configured for **`:latest`**, because image references are resolved independently until **`latest`** is removed or overwritten.

**Tried:** User chose **`9.0.0.0`** over **`9.0.1.0`** for the standardized Quay tag; expects no impact on clusters already pointed at **`latest`**.

**Result:** Agreed for **additive** tagging and unchanged MTV image fields. **Ordering:** migrate ForkliftController / cluster refs to **`…:9.0.0.0`** when ready; only then remove or stop maintaining **`latest`** if the goal is a single source of truth.

### 2026-05-14 — NVIDIA GPU Operator `ClusterPolicy` (runbook step 5)

**Hypothesis:** Instructions mean the **NVIDIA GPU Operator** cluster-scoped **`ClusterPolicy`** (API group **`nvidia.com`**) — *not* ACM/Gatekeeper “cluster policy.” Without `sandboxWorkloads.enabled` and the vGPU-related operands, nodes labeled `vm-vgpu` will not get vGPU Manager / device manager / sandbox device plugin as intended.

**Tried:** User at “update the cluster policy” step per their guide (aligned with [NVIDIA GPU Operator with OpenShift Virtualization — ClusterPolicy](https://docs.nvidia.com/datacenter/cloud-native/openshift/latest/openshift-virtualization.html#creating-a-clusterpolicy-for-the-gpu-operator-using-the-openshift-container-platform-cli)).

**Result:** Operational checklist below; tick step 5 when `oc get clusterpolicy` shows desired spec and operands are `Running` in `nvidia-gpu-operator`.

**`ClusterPolicy` essentials for KubeVirt + vGPU**

| Area | Set / verify |
|------|----------------|
| Sandbox workloads | `spec.sandboxWorkloads.enabled: true` (required so `nvidia.com/gpu.workload.config` on nodes is honored) |
| Sandbox device plugin | `spec.sandboxDevicePlugin.enabled: true` |
| vGPU Manager | `spec.vgpuManager.enabled: true`, plus `repository`, `image` (e.g. `vgpu-manager`), `version` / tag matching your private Quay image, and `imagePullSecrets` if needed |
| vGPU Device Manager | `spec.vgpuDeviceManager.enabled: true` |
| Passthrough only | `spec.vfioManager.enabled: true` — for **vGPU** path do **not** treat VFIO as the primary switch (passthrough vs vGPU is mutually exclusive per node workload class) |

**Typo map (your list → CR fields):** `sandbolorkloads` → **`sandboxWorkloads`**; `sandboxDeviceplugin` → **`sandboxDevicePlugin`**; `vgpuMlanager` → **`vgpuManager`**; `cnabled` → **`enabled`**.

**Example `spec` fragment (YAML)** — merge into existing `ClusterPolicy`; replace placeholders.

```yaml
spec:
  sandboxWorkloads:
    enabled: true
  sandboxDevicePlugin:
    enabled: true
  vgpuManager:
    enabled: true
    repository: quay.example.com/your-org   # registry host + path prefix (per your operator CSV / doc)
    image: vgpu-manager
    version: "REPLACE_ME"                   # vGPU Manager image tag / driver version string (per your build; not MTV VDDK)
    imagePullSecrets:
      - private-registry-secret             # name of ImagePullSecret in nvidia-gpu-operator (if required)
  vgpuDeviceManager:
    enabled: true
```

Confirm field names against your installed CRD: `oc explain clusterpolicy.spec.vgpuManager` (paths can vary slightly by GPU Operator version).

**CLI sketch (baseline from installed CSV, then edit)**

```bash
NS=nvidia-gpu-operator
CSV=$(oc get csv -n "$NS" -o json | jq -r '.items[] | select(.metadata.name | ascii_downcase | contains("gpu-operator")) | .metadata.name' | head -1)
oc get csv -n "$NS" "$CSV" -o jsonpath='{.metadata.annotations.alm-examples}' | jq '.[0]' > clusterpolicy.json
# Edit clusterpolicy.json, then:
oc apply -f clusterpolicy.json
oc get clusterpolicy -o wide
oc get pods -n "$NS"
```

** Preconditions:** GPU Operator subscription installed in **`nvidia-gpu-operator`**; vGPU Manager image in registry; nodes intended for vGPU labeled **`nvidia.com/gpu.workload.config=vm-vgpu`** (checklist step 3). If `ClusterPolicy` already exists, prefer **`oc get clusterpolicy -o yaml`** → merge changes → **`oc apply`** instead of duplicating the CR.

---

## Resolution

_(Fill when resolved.)_

**Root cause:**

**Fix:**

**Prevention:**
