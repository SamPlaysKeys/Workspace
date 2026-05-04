# Homelab Infrastructure Repo Concept

## Architecture Overview

**Infrastructure Layer:** Terraform + Ansible
- Terraform: Provision VMs, network config, base resources
- Ansible: System-level config, package updates, Komodo Periphery agent installation

**Container Management Layer:** Komodo
- Komodo Core runs on dedicated NUC ("Komodo Controller")
- Komodo Periphery agents on all container-running nodes
- GitOps via ResourceSync — changes in Git auto-deploy when `deploy: true`

**Promotion Model:**
- Move TOML file from `test/` → `prod/` to promote
- Copy TOML file to deploy same app in multiple environments

---

## Repo Structure

```
infrastructure/
├── terraform/
│   ├── prod/
│   ├── test/
│   └── modules/
│
├── ansible/
│   ├── inventory/
│   │   ├── prod.yml
│   │   └── test.yml
│   ├── playbooks/
│   │   ├── base-setup.yml        # Common system config
│   │   ├── install-periphery.yml # Install Komodo agent
│   │   └── ...
│   └── roles/
│
└── komodo-configs/
    ├── _stacks/                   # Compose files for grouped apps
    │   ├── arr-suite/
    │   │   └── compose.yaml       # Sonarr, Radarr, Prowlarr, etc.
    │   ├── monitoring/
    │   │   └── compose.yaml       # Grafana, Prometheus, etc.
    │   └── ...
    │
    ├── prod/
    │   ├── servers.toml           # Server definitions for prod
    │   ├── stacks/
    │   │   ├── arr-suite.toml     # Points to _stacks/arr-suite
    │   │   └── plex.toml
    │   └── apps/
    │       ├── homepage.toml      # Individual app configs
    │       └── ...
    │
    ├── test/
    │   ├── servers.toml
    │   ├── stacks/
    │   └── apps/
    │
    ├── dev/
    │   ├── servers.toml           # DevDocker VM server definition
    │   ├── stacks/
    │   └── apps/
    │
    └── shared/
        └── variables.toml         # Shared variables across envs
```

---

## Key Concepts

### Stacks (Grouped Apps)
Compose files in `_stacks/` define groups of related containers.
TOML files in `{env}/stacks/` reference them and set environment-specific config.

```toml
# prod/stacks/arr-suite.toml
[[stack]]
name = "arr-suite"
[stack.config]
server = "prod-docker-01"
file_paths = ["../../_stacks/arr-suite/compose.yaml"]
deploy = true
auto_update = true
```

### Individual Apps (ResourceSync)
Single-app deployments managed as individual TOML files.

```toml
# prod/apps/homepage.toml
[[deployment]]
name = "homepage"
[deployment.config]
server = "prod-docker-01"
image = "ghcr.io/gethomepage/homepage:latest"
deploy = true
```

### Promotion Flow
```
dev/apps/new-app.toml  →  (validate)  →  test/apps/new-app.toml  →  (validate)  →  prod/apps/new-app.toml
       │                                        │                                         │
       └── copy or move ────────────────────────┴── copy or move ─────────────────────────┘
```

---

## Komodo Controller (Dedicated NUC)

- Runs Komodo Core only
- Network access to all environments (prod, test, dev)
- No workloads — purely management plane
- Potentially also runs: Komodo Mongo, any alerting integrations

