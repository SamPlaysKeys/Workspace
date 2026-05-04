# Repository Structure

Layout for the infrastructure-as-code repository that manages the homelab.

---

## Overview

The infrastructure repository has three main sections:

| Section | Tool | Purpose |
|---------|------|---------|
| `terraform/` | Terraform | Provision VMs, networks, base infrastructure |
| `ansible/` | Ansible | System configuration, package management, agent installation |
| `komodo-configs/` | Komodo | Container orchestration via GitOps |

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
└── komodo-configs/
    ├── _stacks/                      # Shared Compose files
    │   ├── arr-suite/
    │   │   └── compose.yaml
    │   ├── monitoring/
    │   │   └── compose.yaml
    │   └── ...
    │
    ├── prod/
    │   ├── servers.toml              # Server definitions
    │   ├── stacks/
    │   │   ├── arr-suite.toml
    │   │   └── plex.toml
    │   └── apps/
    │       ├── homepage.toml
    │       └── ...
    │
    ├── test/
    │   ├── servers.toml
    │   ├── stacks/
    │   └── apps/
    │
    ├── dev/
    │   ├── servers.toml
    │   ├── stacks/
    │   └── apps/
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

## Komodo Configs Section

Defines what containers run where.

### Stacks (`_stacks/`)

Shared Docker Compose files for groups of related containers. These are referenced by environment-specific TOML files.

```yaml
# _stacks/arr-suite/compose.yaml
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

### Environment Directories (`prod/`, `test/`, `dev/`)

Each environment has:
- `servers.toml` — Defines the servers (nodes) in that environment
- `stacks/` — TOML files that reference shared Compose files
- `apps/` — TOML files for individual app deployments

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

**Stack reference example:**

```toml
# prod/stacks/arr-suite.toml
[[stack]]
name = "arr-suite"
description = "Media automation stack"
tags = ["media", "prod"]

[stack.config]
server = "prod-docker-01"
file_paths = ["../../_stacks/arr-suite/compose.yaml"]
deploy = true
auto_update = true
```

**Individual app example:**

```toml
# prod/apps/homepage.toml
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

Moving apps between environments is a file operation:

```
dev/apps/new-app.toml
        │
        ▼ (validate in dev)
test/apps/new-app.toml
        │
        ▼ (validate in test)
prod/apps/new-app.toml
```

**To promote:** Copy or move the TOML file to the next environment's directory. Update the `server` field to point to the correct host.

**To deploy to multiple environments:** Copy the file to each environment (don't move). Each copy points to its respective server.

---

## Komodo Controller

The Komodo Controller (dedicated NUC) runs:
- **Komodo Core** — The control plane that polls Git and orchestrates deployments
- **MongoDB** — Komodo's data store
- **Alerting integrations** — (optional) Slack, Discord, email notifications

The Controller has network access to all managed environments (Prod, Test, Dev) but runs no workloads itself.
