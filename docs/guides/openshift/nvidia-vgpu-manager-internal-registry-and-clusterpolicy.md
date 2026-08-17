---
type: Reference
status: Active
category: Guides
---
# NVIDIA vGPU Manager: build, integrated registry, and ClusterPolicy (OpenShift Virtualization)

This note validates a common on-cluster workflow against NVIDIA’s published steps, then records that workflow when **Quay (or another external registry) is not used** and images land in the **integrated OpenShift image registry** instead.

**Primary NVIDIA reference:** [NVIDIA GPU Operator with OpenShift Virtualization — Building the vGPU Manager image](https://docs.nvidia.com/datacenter/cloud-native/openshift/latest/openshift-virtualization.html#building-the-vgpu-manager-image) and [Creating a ClusterPolicy](https://docs.nvidia.com/datacenter/cloud-native/openshift/latest/openshift-virtualization.html#creating-a-clusterpolicy-for-the-gpu-operator-using-the-openshift-container-platform-cli).

**Related workspace guide:** [Using the integrated OpenShift container image registry](using-integrated-openshift-registry.md).

---
## End-to-end workflow (integrated registry)

### 1. Download the vGPU Manager `.run`

- NVIDIA Licensing Portal → Software Downloads → **Driver downloads** → **Linux KVM** complete vGPU package for the **Product Version** you intend to support.
- Unzip and locate **`NVIDIA-Linux-x86_64-*-vgpu-kvm.run`** (or the AIE variant renamed per NVIDIA).

### 2. Clone NVIDIA’s build context and copy the installer

```bash
git clone https://gitlab.com/nvidia/container-images/driver
cd driver
```

Change into the **`vgpu-manager`** subdirectory for your OS line (NVIDIA’s OpenShift example historically used **`vgpu-manager/rhel8`**; confirm the right path for your OCP / RHCOS generation):

```bash
cd vgpu-manager/rhel8    # example only — pick the path NVIDIA documents for your release
cp /path/to/extracted/NVIDIA-Linux-x86_64-*-vgpu-kvm.run ./
```

### 3. Point the build at the integrated registry and RHCOS

Set **`VERSION`** to the **vGPU software / driver version** from the portal (NVIDIA’s examples look like `510.73.06`; use **your** shipped version string).

Set **`OS_TAG`** to **`rhcos4.<minor>`** matching **your** OpenShift workers (NVIDIA documents this pattern).

Set **`PRIVATE_REGISTRY`** to the registry host and namespace prefix you will **push** to. For the integrated registry, that is commonly the **external route hostname** (for pushes from your workstation) plus project path, for example:

`default-route-openshift-image-registry.apps.<cluster-base>/<project>`

…where **`<project>`** is often `nvidia-gpu-operator` or a dedicated images project. See [using-integrated-openshift-registry.md](using-integrated-openshift-registry.md) for login and push mechanics.

Example (illustrative only — adjust host, project, and versions):

```bash
export VERSION=510.73.06
export OS_TAG=rhcos4.15
export PRIVATE_REGISTRY=default-route-openshift-image-registry.apps.cluster.example.com/nvidia-gpu-operator
```

Build and push using **`podman`** or **`docker`** as in NVIDIA’s doc:

```bash
podman build --build-arg DRIVER_VERSION="${VERSION}" \
  -t "${PRIVATE_REGISTRY}/vgpu-manager:${VERSION}-${OS_TAG}" .
podman push "${PRIVATE_REGISTRY}/vgpu-manager:${VERSION}-${OS_TAG}"
```

Use a **meaningful tag** (here `${VERSION}-${OS_TAG}`) so `ClusterPolicy` and support tickets stay unambiguous.

### 4. Reference the image from in-cluster pulls

Operands pull using the **in-cluster** registry DNS (typically `image-registry.openshift-image-registry.svc:5000/<project>/vgpu-manager:<tag>`). Your **`ClusterPolicy` `vgpuManager.repository` / `image` / `version`** (or equivalent fields) must match how **your installed GPU Operator version** assembles the pull spec. Always confirm with:

```bash
oc explain clusterpolicy.spec.vgpuManager --recursive
```

Conceptually you are wiring **`repository`** + **`image`** + **`version`** so they resolve to the same image you pushed (often **`image`: `vgpu-manager`**, **`version`**: tag string like **`510.73.06-rhcos4.15`** if that is what you pushed).

### 5. Update `ClusterPolicy` (operator already installed)

Generate or export the example **`ClusterPolicy`**, then merge at least:

- **`spec.sandboxWorkloads.enabled: true`**
- **`spec.sandboxDevicePlugin.enabled: true`**
- **`spec.vgpuManager.enabled: true`** with **`repository`**, **`image`**, **`version`**, and **`imagePullSecrets`** if your registry requires them
- **`spec.vgpuDeviceManager.enabled: true`**

NVIDIA’s JSON-oriented example is in [Creating a ClusterPolicy for the GPU Operator using the CLI](https://docs.nvidia.com/datacenter/cloud-native/openshift/latest/openshift-virtualization.html#creating-a-clusterpolicy-for-the-gpu-operator-using-the-openshift-container-platform-cli). If a **`ClusterPolicy`** already exists, prefer **`oc get clusterpolicy -o yaml`**, edit, and **`oc apply`**.

### 6. Verify

```bash
oc get clusterpolicy -o yaml | less
oc get pods -n nvidia-gpu-operator
oc get nodes -l nvidia.com/gpu.workload.config=vm-vgpu
```

When the **vGPU Manager** and **sandbox** operands are healthy and nodes are labeled, continue with NVIDIA’s **HyperConverged** mediated device steps and VM attachment.

---

## Common pitfalls

- **Wrong `vgpu-manager` subdirectory or `OS_TAG`** for your RHCOS line → build succeeds but driver mismatches the node OS.
- **`ClusterPolicy` fields do not match the pushed tag** → `ImagePullBackOff` on daemonsets or operator-managed pods.
- **Image in project A, operands in `nvidia-gpu-operator`** without **`system:image-puller`** (or a pull secret) → pull forbidden across namespaces.
- **Enabling `vgpuManager` before the image exists** → transient failures; enable or point after push.

---

## References

- [NVIDIA GPU Operator with OpenShift Virtualization](https://docs.nvidia.com/datacenter/cloud-native/openshift/latest/openshift-virtualization.html)
- [NVIDIA driver container repository](https://gitlab.com/nvidia/container-images/driver)
- [Red Hat: Accessing the registry](https://docs.redhat.com/en/documentation/openshift_container_platform/latest/html/registry/accessing-the-registry)
