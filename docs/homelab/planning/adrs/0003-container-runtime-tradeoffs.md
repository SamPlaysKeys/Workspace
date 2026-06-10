---
type: ADR
Status: Proposed
Date: 2026-06-09
---

# ADR: Container Runtime Tradeoffs — Docker (Rootful), Docker (Rootless), and Podman

## Context

Selecting a container runtime for baremetal Docker hosts involves balancing security, operational convenience, and tooling compatibility. This ADR breaks down three approaches and the fundamental tension between **central-socket management** and **daemonless isolation**.

The host OS decision is handled in [ADR 0002: Baremetal Docker Host OS Selection](0002-baremetal-host-os.md).

## Runtime Options

### Docker (Rootful)

The classic model: `dockerd` runs as root, exposes `/var/run/docker.sock`.

| Aspect | Detail |
|--------|--------|
| **Socket** | `/var/run/docker.sock` — single, system-wide, owned by `root:docker` |
| **Daemon** | `dockerd` running as root — always-on, manages all containers |
| **Root model** | Container processes run as root by default (can drop to user with `USER` in Dockerfile) |
| **Komodo support** | First-class — native socket access, all features work |
| **Setup complexity** | Minimal — `dnf install docker`, start daemon, add user to `docker` group |
| **Compose** | Native `docker compose` v2 support |

### Docker (Rootless)

`dockerd` runs under a user namespace, exposes a user-scoped socket.

| Aspect | Detail |
|--------|--------|
| **Socket** | `/run/user/$UID/docker.sock` — per-user, still a single management endpoint |
| **Daemon** | `dockerd-rootless` running as the user — still a central daemon |
| **Root model** | Containers run in a user namespace — real root is not exposed, but root *within* the namespace |
| **Komodo support** | Viable with configuration — `DOCKER_HOST` env pointed at rootless socket |
| **Setup complexity** | Moderate — `dockerd-rootless-setuptool.sh`, `loginctl enable-linger`, env setup |
| **Compose** | Native `docker compose` v2 — works within user namespace |

### Podman (Daemonless)

No central daemon. Each container is launched directly by the user's Podman client.

| Aspect | Detail |
|--------|--------|
| **Socket** | None by default — containers are child processes of the user's shell |
| **Daemon** | No daemon — `podman system service` can emulate a Docker API socket on demand |
| **Root model** | Rootless by default — no user namespace needed, containers run as the calling user |
| **Komodo support** | Workarounds only — `DOCKER_HOST` to emulated socket, known bugs (stack status, network labels) |
| **Setup complexity** | Minimal for CLI — `dnf install podman`. Complex for management tooling (need `podman system service` as a systemd unit) |
| **Compose** | `podman-compose` or `podman play kube` — not drop-in compatible |

## Analysis

### Security: The Central Socket Risk

A Docker socket is equivalent to **root access on the host**. Any process that can write to the socket can:

- Start containers with host filesystem mounts (read/write any file)
- Create privileged containers that break out of isolation
- Modify iptables, network namespaces, cgroups
- Access secrets mounted into any container

**Rootful Docker** places this power on a single, well-known path (`/var/run/docker.sock`). The `docker` group is effectively `root`. The blast radius of a compromised process with socket access is **the entire host**.

**Rootless Docker** mitigates this through user namespaces. The daemon and all containers run within the user's namespace — root inside a container is the user outside. A compromised socket can't escape the user namespace, so the blast radius is **that user's resources only**. However, it's still a **central daemon** — if the rootless `dockerd` process is exploited, all that user's containers are compromised.

**Podman** eliminates the central socket entirely. No process management attachment, no daemon to target. Each container is a child of its launching process. The blast radius is **per container** — no socket to poison. To use management tooling, you must explicitly start a `podman system service` socket, which reintroduces a central endpoint — but it's opt-in, not the default.

### Management: Daemonless vs. Central Socket

This is the inverse of the security tradeoff:

| Runtime | Management model | Tooling friction |
|---------|-----------------|------------------|
| Docker (rootful) | One socket, one daemon, everything works | Zero — every tool speaks Docker API natively |
| Docker (rootless) | One socket per user, one daemon per user | Low — set `DOCKER_HOST`, same API |
| Podman | No socket, no daemon, containers are child processes | Higher — no standard API without emulation layer |

**Without a central socket**, management tooling faces a discovery problem:

- How does Komodo's Periphery find the container runtime? It can't — it needs an explicit path to `podman system service` socket, which must be running as a systemd user service.
- How does `docker ps` / `podman ps` enumerate running containers across a session? Podman uses a per-user boltdb database, but there's no persistent daemon registering container state.
- How do compose stacks get tracked? Podman's compose support (`podman-compose`) uses a different project model than Docker Compose — Komodo's stack status detection breaks (issue #504).

**With a central socket**, management is trivial: connect to the well-known path, issue standard API calls, get predictable results. The cost is the security risk described above.

### Operational Considerations

| Criterion | Docker (rootful) | Docker (rootless) | Podman (daemonless) |
|-----------|-----------------|-------------------|---------------------|
| Komodo compatibility | Native | Configurable | Workarounds |
| Socket security | Dangerous (root equiv.) | Safer (user namespaced) | None by default |
| Privileged containers | Full support | Blocked | Blocked (rootless) |
| Port <1024 | Native | Workaround needed | Workaround needed |
| Systemd integration | Manual | Manual | Native (`podman generate systemd`) |
| cgroup management | Full | Limited | Limited (rootless) |
| Builds | Native | Native (BuildKit) | Native (Buildah) |
| Swarm mode | Supported | Blocked | N/A |
| Docker Compose | Native | Native | Partial (podman-compose) |
| Multi-arch builds | Native | Native | Native (Buildah) |

## Recommended Direction

This ADR does not make a standalone decision — it informs [ADR 0002](0002-baremetal-host-os.md). The current recommendation aligns there:

> **Docker (rootless)** as the compromise: a central socket for clean management, but user-namespaced to contain blast radius.

Podman is the more secure architecture by design, and Docker rootful is the most convenient. Rootless Docker sits in the middle — sacrificing some convenience (setup steps, privileged workloads) and some security (still a central daemon) to get a workable balance.

If Podman's Komodo compatibility improves to first-class status, this should be revisited — the daemonless model is genuinely superior for security.

## Consequences

**If Docker (rootless) is adopted (current direction):**

- Management tooling (Komodo, CLI) works through a single socket — simple, standard API
- Central daemon is still a single point of trust — compromise means all that user's containers are at risk
- Setup is more involved than rootful Docker
- Some workloads (privileged containers, low ports) need adaptation

**If Podman is reconsidered later:**

- No central daemon to attack — fundamentally more secure
- Each container is isolated at the process level
- Management complexity is higher — socket emulation required for Komodo
- Tooling ecosystem is still catching up

## References

- Docker: [Rootless Docker](https://docs.docker.com/engine/security/rootless/)
- Docker: [Docker socket security](https://docs.docker.com/engine/security/#docker-daemon-attack-surface)
- Podman: [Rootless Podman](https://github.com/containers/podman/blob/main/docs/tutorials/rootless_tutorial.md)
- Podman: [Systemd integration](https://github.com/containers/podman/blob/main/docs/tutorials/systemd.md)
- Komodo issue #504 — Podman compose network status mismatch
- ADR 0002: [Baremetal Docker Host OS Selection](0002-baremetal-host-os.md)
