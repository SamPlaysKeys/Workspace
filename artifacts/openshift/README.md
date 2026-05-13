# OpenShift artifacts

Reusable OpenShift-related automation: scripts, templates, and playbooks that are safe to copy into other repositories (no client-specific secrets).

## Contents

| Item | Description |
|------|-------------|
| [`readiness-validation-ansible/`](readiness-validation-ansible/) | **Ansible multi-play readiness suite** pattern: parent playbook template, role skeleton, example checks (`check_node_readiness`, `check_storage_health`), full authoring guide (`README.md`), and condensed rules for agents (`rules.md`). |

## Using `readiness-validation-ansible`

1. Open [`readiness-validation-ansible/README.md`](readiness-validation-ansible/README.md) for the full architecture, shared-state contract, OpenShift API gotchas, and CSV matching rules.  
2. Copy `templates/parent_playbook.yml` and `templates/role_skeleton/` into your Ansible project; adapt inventory and extra vars (`cluster_name`, `bastion_fqdn`, `ansible_user`).  
3. Use `examples/` as references when implementing new `readiness_*` roles.

Cross-links: workspace **`docs/guides/openshift/`** holds OpenShift how-tos (registry, GPU Operator, etc.); this directory holds **executable templates** and the long-form readiness authoring spec.
