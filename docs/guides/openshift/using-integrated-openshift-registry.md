---
type: Reference
category: Guides
status: Active
tags:
  - OpenShift
  - Containers
---
# Using the integrated OpenShift container image registry

OpenShift ships with a **cluster-internal container registry** (the **Image Registry Operator**, namespace `openshift-image-registry`). You can push images there instead of an external registry (for example Quay.io), as long as workloads that need the image can **pull** from the namespace where the image lives.

This is the same pattern NVIDIA documents for hosting built images (for example **vGPU Manager**) on the cluster’s own registry.

## What you get

- **In-cluster pull hostname** (from any pod on the cluster):

  `image-registry.openshift-image-registry.svc:5000`

- **Full image reference**:

  `image-registry.openshift-image-registry.svc:5000/<project>/<image>:<tag>`

  Example: `image-registry.openshift-image-registry.svc:5000/nvidia-gpu-operator/vgpu-manager:9.0.0.0`

- **Private by default**: images are not public on the internet; access is via Kubernetes RBAC and image pull credentials where needed.

## Prerequisites

1. **Registry is running** — `ManagementState` should be `Managed`:

   ```bash
   oc get configs.imageregistry/cluster -o jsonpath='{.spec.managementState}{"\n"}'
   ```

2. **Storage** — the registry needs a PVC or other supported backend (install-time or day-2 configuration). If the registry is not ready, pushes fail until storage is fixed.

3. **Pushing from your laptop** — you need a **route** (or port-forward) and authentication. Check for the default route:

   ```bash
   oc get route -n openshift-image-registry
   ```

   If there is no route, an admin can enable the default route or you can push from a **Job** or **Build** inside the cluster that uses the internal service DNS and a service account token.

## Basic workflow: push an image to the integrated registry

1. **Create or pick a project** (namespace) to hold the image, for example `nvidia-gpu-operator` or a dedicated `gpu-images` project.

2. **Log in to the registry** from your workstation (using the **external** route host Red Hat documents for your version), with an OpenShift user token:

   ```bash
   REGISTRY=$(oc get route default-route -n openshift-image-registry -o jsonpath='{.spec.host}')
   podman login -u "$(oc whoami)" -p "$(oc whoami -t)" "$REGISTRY"
   ```

   Use `docker` instead of `podman` if that is what you use.

3. **Tag and push** using the **external** host for the push client (OpenShift documents the exact hostname pattern; it is often `default-route-openshift-image-registry.apps.<cluster-domain>`).

   ```bash
   podman tag mylocal/vgpu-manager:9.0.0.0 "$REGISTRY/nvidia-gpu-operator/vgpu-manager:9.0.0.0"
   podman push "$REGISTRY/nvidia-gpu-operator/vgpu-manager:9.0.0.0"
   ```

4. **Reference the image from manifests** using the **in-cluster** form (`*.svc:5000/...`) so nodes and pods resolve it without depending on the apps route from inside the cluster.

Use **explicit tags** (for example `9.0.0.0` or a driver version string), not only `latest`, so rollbacks and support questions stay clear.

## Pulling from another namespace (for example GPU Operator operands)

If the image lives in project **`gpu-images`** but pods run in **`nvidia-gpu-operator`**, those pods need permission to pull. Typical approaches:

- Grant the **`nvidia-gpu-operator` default service account** (or the operand SA) access to pull from the other namespace (for example `oc policy add-role-to-user system:image-puller system:serviceaccount:nvidia-gpu-operator:default -n gpu-images`), **or**
- Copy/mirror the image into **`nvidia-gpu-operator`** and reference it there, **or**
- Use an **image pull secret** in `nvidia-gpu-operator` that can authenticate to the registry (often unnecessary for same-cluster internal pulls when RBAC is set correctly).

Exact policy depends on your org’s security model; the important part is: **internal registry does not remove the need for RBAC across namespaces.**

## Wiring operators (example: NVIDIA `ClusterPolicy` / `vgpuManager`)

Point **`repository`**, **`image`**, and **`version`** (and **`imagePullSecrets`** if you use them) at the integrated registry path the same way you would for Quay. The operator concatenates fields according to its CRD; confirm with:

```bash
oc explain clusterpolicy.spec.vgpuManager --recursive
```

Conceptually you want the operands to pull something equivalent to:

`image-registry.openshift-image-registry.svc:5000/<project>/<image>:<tag>`

## Verification

```bash
oc get configs.imageregistry/cluster -o yaml | less
oc get route -n openshift-image-registry
oc get imagestream -n nvidia-gpu-operator   # if you use ImageStreams; direct push without IS is also common
```

## Further reading

- Red Hat: [Accessing the registry](https://docs.redhat.com/en/documentation/openshift_container_platform/latest/html/registry/accessing-the-registry) (push/pull, routes, and authentication for your OCP version).

**See also:** [NVIDIA vGPU Manager: build, integrated registry, and ClusterPolicy](nvidia-vgpu-manager-internal-registry-and-clusterpolicy.md) — end-to-end flow when hosting the vGPU Manager image on the integrated registry.
