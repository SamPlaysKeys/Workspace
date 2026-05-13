# Home Lab Deployment Plan: Komodo & GitOps

> **Normative doc:** This scratch note is superseded for architecture and workflow by the **[ADR — Per-environment Git branches via Komodo ResourceSync](../../../docs/homelab/planning/adrs/komodo-resourcesync-branch-per-environment.md)** and the **Git branch steering** section of [Planned repository structure](../../../docs/homelab/platform/planned-repo-structure.md). Keep this file for informal history only.

## 1. Repository Strategy
* **Source of Truth:** The `main` branch serves as the primary source of truth for the entire lab.
* **Environment Separation:** Use a folder-based structure within the repo to separate `dev` and `prod` configurations while keeping both visible across all branches.
* **App-of-Apps Influence:** Utilize Komodo's ResourceSync (via TOML files) to manage stacks and servers declaratively, similar to the Argo CD pattern.

## 2. Branching & Testing Workflow
* **Protected Main:** The `main` branch remains protected.
* **Feature Testing:** When testing a new feature, create a separate branch (e.g., `feature-xyz`).
* **Dynamic Pointer:** To test on the dev host, update the `dev.toml` file in the `main` branch to point its `ref` or `branch` attribute to the active feature branch.
* **Merge:** Once validated, merge the feature branch into `main` and point `dev.toml` back to `main`.

## 3. Repository Layout
/
├── docker/                 # Global Docker Compose templates
│   └── docker-compose.yml
├── dev/                    # Dev-specific environment variables/configs
│   └── .env.dev
├── prod/                   # Prod-specific environment variables/configs
│   └── .env.prod
├── dev.toml                # Komodo ResourceSync for Dev Stack
└── prod.toml               # Komodo ResourceSync for Prod Stack

## 4. Key Mechanism
* **TOML-Controlled Stacks:** Instead of manual UI changes in Komodo, all environment steering is done by editing the TOML files in Git. This keeps the configuration auditable and centralized.
