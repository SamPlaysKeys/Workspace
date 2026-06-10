---
type: ADR
Status: Proposed
Date: 2026-06-09
---

# ADR: Baremetal Docker Host OS Selection

## Context

The homelab needs a host OS for baremetal Docker nodes (LenovoMini 1 for Prod, LenovoMini 2 for Test, and the DevDocker VM on ProxMox). Requirements:

1. **RPM/DNF-based** — user preference, already familiar with the ecosystem.
2. **Komodo GitOps compatibility** — Periphery agent must be able to manage container workloads.
3. **Low-maintenance** — set once and forget; not something to constantly troubleshoot.
4. **Security-conscious** — rootless container runtime preferred if feasible.

## Options Considered

### Host OS

| Option | Release Cadence | Support Window | Notes |
|--------|----------------|----------------|-------|
| **Fedora Server** | ~6mo | ~13mo | Latest kernel + tooling, frequent upgrades |
| **Rocky / Alma Linux** | Major (9.x) | ~10yr | RHEL-compatible, boring & stable |
| **Fedora CoreOS** | Auto-updating streams | Per stream | Immutable, no package management at runtime |
| **CentOS Stream** | Rolling between RHEL releases | Continuous | Between Fedora and RHEL |

### Container Runtime

| Factor | Docker | Podman |
|--------|--------|--------|
| Komodo Periphery support | **First-class** (native socket or API) | **Rough** — community workarounds only (`DOCKER_HOST` to socket emulation) |
| Rootless | Available (opt-in via `dockerd-rootless-setuptool.sh`) | Built-in, rootless by default |
| Compose | Docker Compose v2 | `podman-compose` or `podman play kube` |

## Research Findings

### Podman + Komodo Compatibility
After investigation, Podman support in Komodo relies on workarounds:
- Requires `DOCKER_HOST` env pointing at Podman's Docker API socket (`/run/user/$UID/podman/podman.sock`)
- Stacks may incorrectly show as "DOWN" after successful deploy due to compose network label differences (GitHub issue #504)
- No first-class testing or documentation from Komodo maintainers
- **Conclusion:** Support is too rough for a low-maintenance setup

### Rootless Docker + Komodo
Viable but community-driven — no first-class docs from Komodo, but several working configurations exist:
- **Most robust path:** Periphery as a **systemd user service** pointed at the rootless socket (avoids container-in-Docker filesystem complications)
- Built-in `setup-periphery.py --user` handles the systemd user service install
- Alternate approach: containerized Periphery via `linuxserver/socket-proxy` with `DOCKER_HOST=tcp://socket-proxy:2375` (requires custom image build for non-root git config)
- Komodo maintainer recommends systemd agent over containerized for simpler semantics
- **Inherited rootless Docker limitations:** no `--privileged`, no cgroup limits, no ports <1024 without workarounds, no Swarm mode

### Host OS Alignment
- Fedora Server offers good rootless Docker support and ships recent kernel + DNF
- Rocky/Alma would work but kernel/packages are older; rootless Docker may need backported patches
- Fedora CoreOS is immutable — no package install at runtime, conflicts with ad-hoc debugging approach

## Decision (Provisional)

1. **Fedora Server** for the host OS — DNF-native, recent kernel for rootless Docker, familiar tooling.
2. **Docker** (not Podman) for the container runtime — Komodo's first-class support outweighs the rootless convenience of Podman.
3. **Rootless Docker** on the host — runs without root privileges, aligns with security goals.
4. **Periphery as a systemd user service** — connected to the rootless Docker socket, avoiding container-in-Docker complexity.

## Implementation: Systemd Periphery Setup

Periphery will be installed as a **systemd user service** to interface with rootless Docker. The maintainer recommends systemd over containerized Periphery for simpler Docker semantics (discussion #220), and FoxxMD confirmed this after 3+ months of production use.

### Setup Steps

1. **Prerequisites:**
   - Rootless Docker installed and verified (`dockerd-rootless-setuptool.sh`)
   - `loginctl enable-linger $USER` — required for user services to survive logout/reboot
   - Docker CLI accessible in the user's `$PATH`

2. **Install systemd user service:**
   ```bash
   curl -sSL https://raw.githubusercontent.com/moghtech/komodo/main/scripts/setup-periphery.py \
     | python3 - --user \
       --core-address="https://<core-address>" \
       --connect-as="$(hostname)" \
       --onboarding-key="O-..."
   ```
   The `--user` flag installs to `~/.config/systemd/user/periphery.service`.

3. **Point at rootless Docker socket:**
   Create `~/.config/systemd/user/periphery.service.d/override.conf`:
   ```ini
   [Service]
   Environment="DOCKER_HOST=unix:///run/user/1000/docker.sock"
   Environment="DOCKER_DATA=/home/<user>/docker-data"
   ```
   Then: `systemctl --user daemon-reload && systemctl --user restart periphery`

4. **Enable at boot:**
   ```bash
   systemctl --user enable periphery
   ```

### Known Gotchas

1. **`$UID` does not expand in systemd unit files.** Environment values in `override.conf` use literal strings — no variable interpolation. You must use the numeric UID (e.g., `/run/user/1000/docker.sock`), not `$UID`, `%u`, or `$(id -u)`.

2. **SELinux enforcing on Fedora Server may block Periphery's ptrace.** Periphery uses `ptrace` for process monitoring (container exec, stats). Verify with `ausearch -m avc` after startup. If denials appear, options include: setting a SELinux boolean, writing a custom policy module, or setting `security_opt: apparmor=unconfined` for target containers.

3. **Binary updates are manual.** Containerized Periphery updates by pulling a new image. Systemd Periphery requires re-running `setup-periphery.py` or manually replacing the binary at the installed path, then restarting the service. The script is idempotent (won't change existing config after first run) but is an extra step to remember.

4. **Terminal access model differs from containerized.** FoxxMD's breakdown:
   - Container Periphery → shell is *inside the container*, can interact with Docker daemon but not host
   - Systemd (root) → shell is `root` on the host, full access
   - Systemd (user) → shell is the unprivileged Docker user, access to Docker + user's host files
   With rootless Docker + `--user` install, Komodo's terminal feature grants access as the Docker user — no root host access without `sudo`.

5. **Environment vars don't inherit from shell profile.** `DOCKER_HOST`, `DOCKER_DATA`, and any compose portability variables must be set in the service unit's `Environment=` or `override.conf`. The user's `.bashrc`/`.profile` is invisible to systemd user services.

6. **User namespace UID remapping affects bind mounts.** Rootless Docker maps the host user to UID 0 inside the container, but all other UIDs are shifted. Persistent data directories mounted via compose must be writable by the host user (not `root`), or permission errors will occur. Test volume mounts with `docker run --rm -v /host/path:/container/path alpine touch /container/path/test`.

7. **`setup-periphery.py` may need local inspect before running.** The script checks for `docker` in `$PATH` and assumes certain tools are available. Review the script before running in a rootless-only environment to confirm it doesn't hardcode root paths or expect rootful Docker socket access.

8. **Port conflict on 9120.** Periphery listens on port 8120 by default. If another service uses this port, change it in `periphery.config.toml`. With rootless Docker, ports above 1024 are fine; 8120 is above the threshold.

## Consequences

**Positive:**
- Familiar RPM/DNF management
- Komodo Periphery has native, well-documented support for Docker
- Rootless Docker reduces attack surface of the container runtime
- systemd agent avoids nested filesystem issues and Docker CLI version mismatches

**Negative / risks:**
- Rootless Docker requires additional setup vs. rootful (`dockerd-rootless-setuptool.sh`, `loginctl enable-linger`, environment variables)
- No `--privileged` containers — some workloads (e.g., `socket-proxy`, certain networking tools) may need adaptation
- Ports below 1024 require `sysctl` or auth bind workaround
- Fedora ~13mo support window means OS upgrades every ~6-12mo (acceptable for homelab)
- Rootless Docker + systemd Periphery is community-tested with Komodo, not officially documented
- Systemd Periphery has several implementation gotchas — see [Implementation: Systemd Periphery Setup](#implementation-systemd-periphery-setup) above for full breakdown

## Open Questions

- How to handle privileged workloads (e.g., Tailscale container, Docker socket proxies) under rootless Docker?
- What's the port <1024 strategy for web services on baremetal hosts? (Reverse proxy on the NUC? `net.ipv4.ip_unprivileged_port_start=443`?)
- BuildKit + rootless for multi-stage builds — any friction?

## References

- ADR 0003: [Container Runtime Tradeoffs](0003-container-runtime-tradeoffs.md) — deeper analysis of Docker rootful, rootless, and Podman comparison
- FoxxMD — [Migrating to Komodo](https://blog.foxxmd.dev/posts/migrating-to-komodo/#create-komodo-periphery-agents) (rootless Periphery via socket-proxy, systemd agent recommendation)
- Docker — [Rootless Docker setup](https://docs.docker.com/engine/security/rootless/)
- Komodo — [Periphery installation docs](https://komo.do/docs/setup/connect-servers)
- Komodo issue #504 — Podman compose network status mismatch
- Komodo discussion #220 — Maintainer recommends systemd agent over containerized
