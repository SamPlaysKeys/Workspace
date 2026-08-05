---
type: Guide
category: OpenShift Learning Path
status: Active
---
# Module 2 — Containers & Registries

**Audience:** Engineers new to containerization, or coming from VM-centric operations.

**Outcomes:** Build and run container images; understand the layer model, image digests, and registries; reason about base images, tags vs digests, and registry authentication — the foundation for Kubernetes (Module 3).

---

## Checklist — work through in order

- [ ] Read the mental model below (images, layers, tags vs digests, Podman vs Docker)
- [ ] **Learn containers before Kubernetes** — do a Docker/Podman fundamentals course: KodeKloud "Docker for the Absolute Beginner" or [Podman docs](https://docs.podman.io/) getting-started
- [ ] Build and run your first image from a minimal base (UBI/distroless)
- [ ] **DO180** (Red Hat: Containers, Kubernetes & OpenShift) as the formal path — [free Red Hat Developer tutorials](https://developers.redhat.com/learn/openshift) as a no-cost alternative
- [ ] Practice registry push/pull, tags vs digests, and image scanning <!-- ORG-SPECIFIC: internal registry, CI build pipeline, base-image catalog -->
- [ ] Pass the scenario-based Verification at the bottom

---

## Mental model

A container is an isolated, runnable instance of an **image** — an immutable, layered filesystem plus metadata. Unlike a VM, a container shares the host kernel and has no hypervisor. The unit Kubernetes reasons about is the *container* (packed into *Pods*), not the VM.

Key shifts from VM thinking:
- **Images are immutable artifacts**, not running state. You rebuild, not "snapshot and edit."
- **Layers are content-addressed and shared** — changes only add new layers.
- **Tags are mutable pointers**; **digests are immutable**. Production should pin by digest.

<!-- ORG-SPECIFIC: our standard base images, approved registries, and image-scanning/quarantine policy. -->

## Topics

- **Images & layers** — `Dockerfile` / `Containerfile` instructions; layer caching; minimal base images (UBI, distroless).
- **Building & running** — `build`, `run`, `exec`, `logs`, port/publish, volumes, environment.
- **Registries** — pull/push, tags vs digests, auth, mirror registries, private/internal registries.
- **Image hygiene** — sizing, non-root users, multistage builds, scanning for CVEs.
- **Podman vs Docker** — rootless containers; `podman` is OCI-compliant and often the platform default (no daemon).

## Resources

- **[Red Hat — DO180: Containers, Kubernetes, and Red Hat OpenShift](https://www.redhat.com/en/services/training/do180-introduction-containers-kubernetes-red-hat-openshift)** (formal course; containers + K8s intro).
- **[OpenShift learning (Red Hat Developer)](https://developers.redhat.com/learn/openshift)** — no-cost tutorials (URLs change; use as hub).
- **Podman documentation** — [docs.podman.io](https://docs.podman.io/); `podman build`/`run` mirror Docker CLI.
- <!-- ORG-SPECIFIC: internal registry URL, CI image-build pipeline, base-image catalog. -->

## Verification (scenario-based)

1. Write a `Containerfile` for a small HTTP app from a minimal base image; build it; run it; `curl` the exposed port.
2. Inspect layers (`podman history` / `docker history`); explain which instruction added which layer and why caching matters.
3. Pull an image by **tag**, then by **digest**; explain why the digest is reproducible and the tag is not.
4. Push an image to a registry; explain auth and how a cluster later pulls it (ties to Module 3 image pulls + Module 8 disconnected mirrors).


