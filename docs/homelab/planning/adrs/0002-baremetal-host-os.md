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
4. **Easily automatable** — host provisioning should be reproducible via Ansible, including Periphery agent setup.

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

### Rootful vs Rootless Decision
Rootless Docker was initially preferred for security, but three factors drove the switch to rootful:
- **No practical benefit for a homelab** — all workloads are user-deployed and trusted. The socket is already protected by host-level access controls (SSH, firewall, physical security).
- **Avoids community-driven maintenance burden** — rootless Docker with Komodo Periphery relies on community patterns not official docs. Rootful is the documented, tested path.
- **Simplifies Ansible automation** — rootful Docker is a single `dnf install` + daemon enable. No `dockerd-rootless-setuptool.sh`, no `loginctl enable-linger`, no environment variable wiring.

### Host OS Alignment
- Fedora Server ships recent kernel + DNF, packages are fresh enough for both the Docker CE repo and Ansible management
- Rocky/Alma would work but are conservative; no advantage for this use case
- Fedora CoreOS is immutable — no package install at runtime, conflicts with Ansible-based provisioning

## Decision (Provisional)

1. **Fedora Server** for the host OS — DNF-native, recent kernel, familiar tooling, easy Ansible provisioning.
2. **Docker** (not Podman) for the container runtime — Komodo's first-class support removes any reason to fight Podman compatibility.
3. **Rootful Docker** — simplicity wins for a homelab with trusted workloads. No port restrictions, no privileged container limitations, no extra setup steps.
4. **Containerized Periphery** — deployed as a Docker container managed by Ansible, with `/var/run/docker.sock` mounted for API access. Updates via image pull, not manual binary replacement.

## Implementation: Containerized Periphery via Ansible

Periphery runs as a Docker container, managed by the same Ansible playbook that provisions the host. The Komodo maintainer's filesystem semantics concern (discussion #220) applies to rootless Docker where user namespaces cause path remapping — with rootful Docker, container and host paths are the same, so the concern doesn't apply.

### Ansible Role Structure

The host provisioning playbook will include a role `komodo-periphery` that handles Periphery as a container:

```
roles/
  komodo-periphery/
    tasks/
      main.yml          # pull, create dirs, run container
    templates/
      periphery.config.toml.j2   # config template
    vars/
      main.yml          # defaults (core URL, port, etc.)
```

### Tasks (main.yml)

```yaml
- name: Ensure Periphery data directory exists
  ansible.builtin.file:
    path: /opt/periphery
    state: directory
    mode: '0755'

- name: Deploy Periphery container
  community.docker.docker_container:
    name: periphery
    image: "ghcr.io/moghtech/periphery:{{ periphery_version }}"
    restart_policy: unless-stopped
    network_mode: host
    volumes:
      - "/var/run/docker.sock:/var/run/docker.sock"
      - "/opt/periphery:/config"
      - "/:/mnt/host:ro"
    env:
      PERIPHERY_CONFIG_PATH: /config/periphery.config.toml
```

Key choices:
- **`network_mode: host`** — Periphery needs to reach containers by their host-level ports for monitoring. Host networking avoids port mapping conflicts and simplifies the network topology.
- **`/var/run/docker.sock` mount** — gives Periphery the standard Docker API access. With rootful Docker this is the native socket, no indirection needed.
- **`/:/mnt/host:ro`** — bind mounts the host root so Periphery can read filesystem stats for disk monitoring.
- **`restart_policy: unless-stopped`** — survives reboots and daemon restarts.

### Config Template (`periphery.config.toml.j2`)

```toml
[periphery]
port = {{ periphery_port | default(8120) }}
root_directory = "/mnt/host"

[[connections]]
address = "wss://{{ core_address }}/connection"
connect_as = "{{ inventory_hostname }}"
onboarding_key = "{{ periphery_onboarding_key }}"
```
The config template is rendered by Ansible with host-specific variables (core address, hostname, onboarding key), sourced from Ansible vault or group vars.

### Ansible Playbook Structure

```yaml
- hosts: docker_hosts
  become: yes
  roles:
    - role: docker-install        # dnf install docker, enable + start
    - role: komodo-periphery      # pull image, deploy container
```

### Gotchas

1. **SELinux may block container socket access.** On Fedora Server with SELinux enforcing, the Periphery container needs the `:z` flag on volume mounts or a dedicated SELinux policy. If Periphery fails to connect, check `ausearch -m avc` and add `:z` to the socket mount: `"/var/run/docker.sock:/var/run/docker.sock:z"`.

2. **Host networking limits to one Periphery per host.** With `network_mode: host`, port 8120 is consumed directly on the host. Only one Periphery instance can run per machine. This is the expected topology — one agent per host.

3. **Periphery container image tag strategy.** Use a specific version tag (e.g., `:0.2.5`) not `:latest` for reproducible deployments. Update by bumping the version in Ansible vars and re-running the playbook.

4. **Onboarding key rotates.** The `onboarding_key` in `[[connections]]` is a one-time use key from the Komodo Core UI. After first connection, Periphery persists its session — the key is no longer needed. If re-deploying from scratch, generate a new key.

5. **Config changes require container restart.** If `periphery.config.toml` changes, Ansible must trigger a container restart. The `docker_container` module handles this automatically when config changes are detected via template diff.

6. **Ansible vault for secrets.** The onboarding key is sensitive and should be stored in Ansible vault, not in plaintext group vars.

## Consequences

**Positive:**
- Familiar RPM/DNF management
- Komodo Periphery has native, first-class support for rootful Docker
- All workloads work without restriction — privileged containers, ports <1024, cgroup limits
- Containerized Periphery auto-updates via `docker pull`; Ansible handles version pinning
- Ansible-driven provisioning means new hosts are reproducible
- No special setup beyond standard Docker CE install

**Negative / risks:**
- Docker socket is root-equivalent — compromised container has host access. Mitigated by homelab trust model (all workloads are user-deployed).
- Containerized Periphery adds a Docker-in-Docker layer that the Komodo maintainer flags for potential filesystem confusion, but with rootful Docker (no user namespace) this is not a practical concern.
- Fedora ~13mo support window means OS upgrades every ~6-12mo (acceptable for homelab).
- Periphery config changes need container restart — automated via Ansible.

## Open Questions

- Which Periphery image tag to pin initially? Start with `:latest` during bring-up, pin to a specific version once stable.
- How to handle Periphery version upgrades? Ansible var bump + playbook re-run. Need a process for checking upstream releases.

## References

- ADR 0003: [Container Runtime Tradeoffs](0003-container-runtime-tradeoffs.md) — deeper analysis of Docker rootful, rootless, and Podman comparison
- FoxxMD — [Migrating to Komodo](https://blog.foxxmd.dev/posts/migrating-to-komodo/#create-komodo-periphery-agents) (rootless Periphery via socket-proxy, systemd agent recommendation)
- Docker — [Rootless Docker setup](https://docs.docker.com/engine/security/rootless/)
- Komodo — [Periphery installation docs](https://komo.do/docs/setup/connect-servers)
- Komodo issue #504 — Podman compose network status mismatch
- Komodo discussion #220 — Maintainer recommends systemd agent over containerized
