---
type: README-Note
---
# Ansible Artifacts

Reusable Ansible playbooks, roles, and inventory templates.

> **Note:** Reference artifacts from previous work will be added here once they've been reviewed and cleaned of any potential secrets or sensitive information.

## Contents

- [`aap-template-export/`](./aap-template-export/README.md) — Automation scripts (production Bash, minimal Bash, and Python reference PoC) to query AAP / AWX Controller v2 APIs, enrich Job Templates and Workflow Templates with human-readable Project/Inventory/EE names, and generate CSV/JSON reports.

**OpenShift-focused Ansible:** See [`../openshift/readiness-validation-ansible/`](../openshift/readiness-validation-ansible/README.md) — multi-play cluster readiness validation pattern, role skeleton, and examples.

## Planned

- Base system configuration role
- Docker installation playbook
- Komodo Periphery agent installation (for homelab)
