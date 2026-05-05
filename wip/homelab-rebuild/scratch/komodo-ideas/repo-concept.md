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
└── komodo/
    ├── prod/
    │   ├── servers.toml           # Server definitions for prod
    │   ├── arr-suite/
    │   │   ├── arr-suite.toml
    │   │   └── compose.yaml
    │   ├── plex/
    │   │   ├── plex.toml
    │   │   └── compose.yaml
    │   └── homepage.toml          # Simple apps can be flat
    │
    ├── test/
    │   ├── servers.toml
    │   └── ...
    │
    ├── dev/
    │   ├── servers.toml           # DevDocker VM server definition
    │   └── ...
    │
    └── shared/
        └── variables.toml         # Shared variables across envs
```

---

## Key Concepts

### App Organization

Each app gets its own subdirectory containing all related files:

```
prod/arr-suite/
├── arr-suite.toml    # Komodo config
└── compose.yaml      # Docker Compose definition
```

Subdirectories are for organization — Komodo scans recursively.

### Stacks (Multi-container apps)

```toml
# prod/arr-suite/arr-suite.toml
[[stack]]
name = "arr-suite"
[stack.config]
server = "prod-docker-01"
file_paths = ["compose.yaml"]
deploy = true
auto_update = true
```

### Simple Apps (Single container, flat file)

```toml
# prod/homepage.toml
[[deployment]]
name = "homepage"
[deployment.config]
server = "prod-docker-01"
image = "ghcr.io/gethomepage/homepage:latest"
deploy = true
```

### Promotion Flow
```
dev/new-app/  →  (validate)  →  test/new-app/  →  (validate)  →  prod/new-app/
      │                              │                               │
      └── copy or move folder ───────┴── copy or move folder ────────┘
```

Each environment can have different compose settings or config values.

---

## Komodo Controller (Dedicated NUC)

- Runs Komodo Core only
- Network access to all environments (prod, test, dev)
- No workloads — purely management plane
- Potentially also runs: Komodo Mongo, any alerting integrations

