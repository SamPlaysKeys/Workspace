# Homelab Repository Structure

Layout for the infrastructure-as-code repository that manages the homelab.

---

## Overview

The infrastructure repository has three main sections:

| Section | Tool | Purpose |
|---------|------|---------|
| `terraform/` | Terraform | Provision VMs, networks, base infrastructure |
| `ansible/` | Ansible | System configuration, package management, agent installation |
| `komodo/` | Komodo | Container definitions and GitOps configs |

---

## Full Structure

```
infrastructure/
├── terraform/
│   ├── prod/
│   ├── test/
│   ├── dev/
│   └── modules/
│
├── ansible/
│   ├── inventory/
│   │   ├── prod.yml
│   │   ├── test.yml
│   │   └── dev.yml
│   ├── playbooks/
│   │   ├── base-setup.yml           # Common system config
│   │   ├── install-periphery.yml    # Install Komodo Periphery agent
│   │   └── ...
│   └── roles/
│
└── komodo/
    ├── prod/
    │   ├── servers.toml              # Server definitions
    │   ├── arr-suite/
    │   │   ├── arr-suite.toml
    │   │   └── compose.yaml
    │   ├── plex/
    │   │   ├── plex.toml
    │   │   └── compose.yaml
    │   └── homepage.toml             # Simple apps can be flat
    │
    ├── test/
    │   ├── servers.toml
    │   └── ...
    │
    ├── dev/
    │   ├── servers.toml
    │   └── ...
    │
    └── shared/
        └── variables.toml            # Shared variables across envs
```

---

## Terraform Section

Provisions the underlying infrastructure.

```
terraform/
├── prod/           # Production environment resources
├── test/           # Test environment resources
├── dev/            # Dev environment resources (DevDocker VM)
└── modules/        # Reusable modules
```

**Managed resources:**
- VMs on ProxMox (for DevDocker, DevNode)
- Network configuration
- DNS entries (if applicable)
- Any cloud resources

---

## Ansible Section

Configures systems after provisioning.

```
ansible/
├── inventory/
│   ├── prod.yml    # Prod hosts
│   ├── test.yml    # Test hosts
│   └── dev.yml     # Dev hosts
├── playbooks/
│   ├── base-setup.yml           # OS config, packages, users
│   ├── install-periphery.yml    # Komodo Periphery agent
│   ├── install-docker.yml       # Docker runtime
│   └── ...
└── roles/
    ├── common/
    ├── docker/
    ├── periphery/
    └── ...
```

**Responsibilities:**
- Base system configuration (users, SSH, time, etc.)
- Package installation and updates
- Docker installation
- Komodo Periphery agent installation and configuration
- Any system-level dependencies

---

## Komodo Section

Defines what containers run where.

### Environment Directories (`prod/`, `test/`, `dev/`)

Each environment has:
- `servers.toml` — Defines the servers (nodes) in that environment
- `<app>/` — Subdirectory per app containing all related files (TOML config, compose.yaml, Dockerfiles, etc.)

Subdirectories are for organization only — Komodo scans recursively and finds TOML files regardless of nesting.

**Server definition example:**

```toml
# prod/servers.toml
[[server]]
name = "prod-docker-01"
description = "Production Docker host"
tags = ["prod", "docker"]

[server.config]
address = "http://192.168.1.10:8120"
enabled = true
```

**Stack example (multi-container app):**

```yaml
# prod/arr-suite/compose.yaml
services:
  sonarr:
    image: linuxserver/sonarr:latest
    # ...
  radarr:
    image: linuxserver/radarr:latest
    # ...
  prowlarr:
    image: linuxserver/prowlarr:latest
    # ...
```

```toml
# prod/arr-suite/arr-suite.toml
[[stack]]
name = "arr-suite"
description = "Media automation stack"
tags = ["media", "prod"]

[stack.config]
server = "prod-docker-01"
file_paths = ["compose.yaml"]
deploy = true
auto_update = true
```

**Simple app example (single container, no compose — flat file):**

```toml
# prod/homepage.toml
[[deployment]]
name = "homepage"
description = "Dashboard"
tags = ["dashboard", "prod"]

[deployment.config]
server = "prod-docker-01"
image = "ghcr.io/gethomepage/homepage:latest"
deploy = true
```

### Shared Variables (`shared/`)

Variables that can be interpolated across environments.

```toml
# shared/variables.toml
[[variable]]
name = "TZ"
value = "America/New_York"

[[variable]]
name = "PUID"
value = "1000"

[[variable]]
name = "PGID"
value = "1000"
```

---

## Promotion Flow

Moving apps between environments is a folder operation:

```
dev/new-app/
        │
        ▼ (validate in dev)
test/new-app/
        │
        ▼ (validate in test)
prod/new-app/
```

**To promote:** Copy or move the app's folder to the next environment's directory. Update the `server` field in the TOML to point to the correct host.

**To deploy to multiple environments:** Copy the folder to each environment. Each copy can have environment-specific tweaks (different compose settings, config values, etc.).

---

## Git branch steering

Folder promotion (`dev/` → `test/` → `prod/`) answers **which subtree on `main` is canonical after merge**. **Which Git revision each environment applies** is separate: each Komodo **ResourceSync** sets its own **branch / ref** and scoped paths against the infrastructure repo.

**Homelab default:**

- `main` holds the full `komodo/` tree (every environment directory) for reviewability and docs.
- Each environment has its own ResourceSync so, for example, **Dev** can track `feature/xyz` while **Prod** tracks `main`.
- After merging a feature branch, **manually** repoint ResourceSync metadata on `main` for any environment that should follow `main` again (sync TOML on the feature branch usually names that branch while work is open — see ADR).

**Releases / tags** to pin Prod are optional later; per-environment folders + branch knobs reduce the urgency.

Full rationale, merge quirk, and mitigations: **[ADR — Per-environment Git branches via Komodo ResourceSync](../planning/adrs/komodo-resourcesync-branch-per-environment.md)**.

---

## Komodo Controller

The Komodo Controller (dedicated NUC) runs:
- **Komodo Core** — The control plane that polls Git and orchestrates deployments
- **MongoDB** — Komodo's data store
- **Alerting integrations** — (optional) Slack, Discord, email notifications

The Controller has network access to all managed environments (Prod, Test, Dev) but runs no workloads itself.
