---
type: ADR
Status: Accepted
Date: 2026-10-30
Supersedes: 0001
agent_generated: true
agent_at: "2026-10-30T00:00:00Z"
review_status: "awaiting_review"
reviewed_by: ""
last_modified_by: "agent"
---

# ADR: Organize infrastructure by purpose, not environment

## Context

The earlier model had Dev, Test and Prod environments, each Komodo-managed on Fedora + Docker, with a ResourceSync per environment following its own branch ([ADR 0001](0001-komodo-resourcesync-branch-per-environment.md)). The hardware changed: UnRAID-to-TrueNAS migration was cancelled, Proxmox and Synology are gone, and Test/Lab became two clusters. There is no real promotion pipeline between machines.

## Decision

Group machines by what they are for:

| Group | Machine | Role |
|---|---|---|
| Mgmt | `nuclet` | Komodo Core, Semaphore (all Ansible, Terraform, `talosctl`, OS updates) |
| Prod | `meadowprod` (Debian) + TrueNAS (NFS) | Production containers via Komodo |
| Dev | `devbox` (Fedora Server) | See [ADR 0008](0008-dev-host-fedora-cockpit-podman-kvm.md) |
| Test | Talos cluster | Not yet configured |
| Lab | OpenShift cluster | Enterprise testing |
| Remote | Two Windows Lenovo minis | Remote hosts (self and partner) |

- No dev/test/prod promotion model. One Komodo ResourceSync follows `main`.
- Komodo stacks are git-based (compose files committed in the repo, secrets via Komodo Variables). Ansible handles host-level work only.
- Semaphore on `nuclet` replaces the GitHub self-hosted runner for automation.
- The `tailscale/` folder and its workflow are unchanged.

## Consequences

- Simpler repo: `komodo/prod`, `komodo/dev-docker-vm`, `komodo/shared`.
- Changes to `main` deploy directly; there is no soak branch. Use `poll_for_updates`/manual deploys for risky stacks.
- Docs elsewhere in `docs/homelab/` still describe the old environments and need a follow-up pass.

## References

- [ADR 0001](0001-komodo-resourcesync-branch-per-environment.md), [ADR 0008](0008-dev-host-fedora-cockpit-podman-kvm.md)
- `InfrastructureAsCode` repo, branch `overhaul/v2`
