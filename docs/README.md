# Documentation

Graduated documentation from brainstorming sessions, troubleshooting investigations, and project work.

## Structure

```
docs/
├── homelab/           # Homelab planning (see homelab/README.md for topic index)
│   ├── overview/      # Map + visual graphic
│   ├── network/       # Tailscale, UniFi
│   ├── platform/      # Architecture, environments, hardware, repo layout
│   ├── observability/ # Public status + operator dashboard plans
│   ├── planning/      # Decisions, roadmap, scratchpad
│   └── iot/           # Smart home
├── troubleshooting/   # Remediation guides (from Troubleshoot sessions)
├── guides/            # Setup and prevention guides
│   ├── openshift/     # OpenShift / OCP how-tos (registry, GPU Operator, etc.)
│   └── dev-environment/ # VSCode setup, local tools, etc.
├── blog/              # Blog post drafts
└── notes/             # Personal notes (events, etc.)
```

## How Documentation Gets Here

Documentation graduates from `wip/` sessions:

1. **Storm Sessions** → Project documentation (e.g., `docs/homelab/`)
2. **Troubleshoot Sessions** → Remediation docs (`docs/troubleshooting/`) and guides (`docs/guides/`)
3. **Reusable automation templates** → May graduate from `wip/` to `artifacts/` (for example Ansible/OpenShift patterns) with documentation alongside the templates

See `workstyle/working_style.md` for the full graduation workflow.

## Current Projects

| Project | Description | Status |
|---------|-------------|--------|
| [Homelab](homelab/) | Infrastructure architecture for greenfield homelab rebuild | Architecture complete |
| [OpenShift readiness (Ansible)](../artifacts/openshift/readiness-validation-ansible/README.md) | Multi-play validation pattern, role skeleton, example roles; long-form authoring guide in-repo | Graduated from `wip/` |
