---
type: README-Note
---
# Artifacts

Reusable code, scripts, and configurations that can be applied across projects.

> **Note:** Reference artifacts from previous work will be added here once they've been reviewed and cleaned of any potential secrets or sensitive information.

## Structure

```
artifacts/
├── ansible/              # Ansible playbooks, roles, and inventory templates
├── articulate-storyline/ # Web Objects for Articulate Storyline courses
├── bash/                 # Shell scripts and utilities
└── openshift/            # OpenShift automation (YAML, Ansible, scripts)
    └── readiness-validation-ansible/  # Multi-play cluster readiness pattern + examples
```

## Usage

Artifacts here are designed to be:
- **Portable** — Can be copied or referenced from other repositories
- **Documented** — Each artifact should explain its purpose and usage
- **Tested** — Where applicable, include validation or test cases

## Contributing Artifacts

When creating reusable artifacts:
1. Place in the appropriate subdirectory
2. Include a header comment explaining purpose and usage
3. Document any dependencies or prerequisites
