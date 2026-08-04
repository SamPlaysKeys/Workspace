---
type: Type
_sidebar_label: ADR
layout: page
title: 'ADR: Podman vs Docker for Container Runtime'
category: Homelab
status: Active
---
{% raw %}


# ADR: Podman vs Docker for Container Runtime

## Status
**Draft**

## Context
We need a container runtime for local development, CI/CD pipelines, and homelab deployment. The ecosystem has two primary open-source contenders: **Docker** (the incumbent) and **Podman** (the rising alternative). Both implement OCI standards and can run the same container images, but differ significantly in architecture, security model, and ecosystem integration.

**Key evaluation criteria:**
- Security posture (rootless vs rootful by default)
- Daemon architecture (monolithic vs fork/exec)
- Docker-compatibility and migration friction
- Desktop GUI experience (Docker Desktop vs Podman Desktop)
- Orchestration integration (Kubernetes, Compose, Swarm)
- Licensing and corporate governance

## Decision
**Defer — no single winner.** The choice depends on the deployment context:

| Context | Recommendation | Why |
|---------|---------------|-----|
| Local dev (macOS) | Docker Desktop | Better UX, tighter IDE integration, mature volume mounts |
| Local dev (Linux) | Podman | Native rootless, no daemon tax, distro-packaged |
| CI/CD pipelines | Docker-in-Docker | Industry standard, broadest runner support |
| Homelab server | Podman | Systemd integration, rootless by default, lighter footprint |
| Kubernetes dev | Podman | Built-in pod concept, better k8s alignment |

## Rationale

### Security
- **Podman**: Rootless by default. Each user gets their own namespace — no central daemon with root-equivalent access. This is a fundamental architectural advantage for multi-tenant systems and security-conscious deployments.
- **Docker**: The daemon (`dockerd`) runs as root by default. Adding users to the `docker` group is effectively root-equivalent. Rootless mode exists but is opt-in and less mature.

### Architecture
- **Docker**: Client-server model. `docker` CLI → `dockerd` daemon → `containerd` → `runc`. The daemon is a single point of failure and a persistent resource drain.
- **Podman**: Fork/exec model. Each `podman` command directly spawns container processes via `conmon` + `runc`/`crun`. No persistent daemon. This means containers are systemd-manageable and don't die if a daemon crashes.

### Docker-Compatibility
- **Podman** implements the Docker CLI API (`alias docker=podman` works for most commands), supports `docker-compose` (via `podman-compose` or `podman machine`), and can pull/build/run any OCI image.
- **Edge cases exist**: Volume mount semantics differ, networking stacks diverge (`podman` uses pasta/slirp4netns vs Docker's built-in bridge), and some Docker-specific features (Swarm mode, BuildKit advanced features) have no Podman equivalent.

### Desktop Experience
- **Docker Desktop**: Polished, all-in-one GUI. Includes Kubernetes cluster, volume management, registry browser, Dev Environments, and extensions ecosystem. Paid license for commercial use >250 employees or $10M+ revenue.
- **Podman Desktop**: Open-source GUI, improving rapidly. Supports Podman and Docker backends, Kubernetes integration, extensions. Lighter but less feature-rich. Free for all use.

### Ecosystem & Governance
- **Docker**: Docker, Inc. Owns the developer mindshare. Vast documentation, tutorials, Stack Overflow presence. Docker Hub is the default registry. Licensing changes (2021 commercial license) caused community friction.
- **Podman**: Red Hat–backed, part of the libpod project. CNI-aligned networking, systemd-native. Growing adoption in enterprise Linux spaces (RHEL, Fedora). No corporate licensing tension.

## Consequences
- We may need to support both runtimes depending on environment. A `docker` alias or abstraction layer (`CONTAINER_RUNTIME` env var) can ease transitions.
- Compose v3 files are broadly portable; v4 features may not be. Stick to v3 for cross-runtime compatibility.
- Podman's macOS experience relies on a Linux VM (via `podman machine` or `podman desktop`), similar to Docker Desktop but less polished. Volume mounts and port forwarding are slower on macOS.
- If Kubernetes is the long-term target, Podman's pod-level semantics map more naturally to k8s concepts.
- Docker is still the default for most CI/CD providers. Using Podman in CI requires explicit runner configuration.

{% endraw %}