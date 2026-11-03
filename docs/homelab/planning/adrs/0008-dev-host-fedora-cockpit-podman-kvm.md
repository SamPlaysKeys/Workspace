---
type: ADR
Status: Accepted
Date: 2026-10-30
Supersedes: 0002
agent_generated: true
agent_at: "2026-10-30T00:00:00Z"
review_status: "awaiting_review"
reviewed_by: ""
last_modified_by: "agent"
---

# ADR: Dev host: Fedora Server with Cockpit, Podman Quadlets and KVM

## Context

[ADR 0002](0002-baremetal-host-os.md) chose Fedora Server with rootful Docker and a containerized Komodo Periphery as the baremetal Docker host, and assumed Proxmox for the dev machine. Since then:

- The dev machine (`devbox`) dropped Proxmox. It runs Fedora Server directly.
- Dev work focuses on infrastructure (VMs, cluster tooling) more than on long-running containers.
- Komodo's Podman support is still rough (issue #504), but dev containers no longer need to be Komodo-managed.

## Decision

1. **`devbox` runs Fedora Server on bare metal.** Management plane is **Cockpit**.
2. **KVM/libvirt** hosts dev VMs, created with Terraform (libvirt provider) and managed in Cockpit. `fedora-dev` and other dev VMs are guests, not the host.
3. **Podman** is the container runtime on the host. Host-level services run as **Quadlets** (systemd units) deployed by Ansible. If Quadlets prove awkward, `podman-compose` is an acceptable fallback.
4. **Komodo does not manage `devbox` itself.** Komodo manages containers only on `meadowprod` (Debian prod) and on a **Docker Test VM** hosted on `devbox`, which runs Docker with Periphery. Other dev VMs are unmanaged by Komodo.
5. Host provisioning and updates are run from Semaphore on `nuclet`.

## Consequences

**Positive**
- No Docker daemon or Periphery on the dev host; fewer moving parts.
- Cockpit gives a UI for VMs, Podman and updates without extra tooling.
- Komodo's Docker-first assumptions stay where they work (Debian prod, Docker Test VM).

**Negative / risks**
- Quadlets need hand-written units; there is no Compose-style GitOps for the dev host.
- Two container runtimes in the lab (Podman on `devbox`, Docker elsewhere).
- Fedora's ~13 month support window still means OS upgrades every 6-12 months.

## What of ADR 0002 still holds

Fedora Server as host OS, DNF-based, Ansible-provisioned. Rootful Docker with Periphery applies only to the Docker Test VM and to Debian prod.

## References

- [ADR 0002](0002-baremetal-host-os.md), [ADR 0003](0003-container-runtime-tradeoffs.md)
- Komodo issue #504 (Podman compose network status mismatch)
- Implementation: `InfrastructureAsCode` repo, branch `overhaul/v2`, `docs/overhaul-plan.md`
