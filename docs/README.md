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
├── blog/              # Blog post drafts
└── presentations/     # Presentation materials
```

## How Documentation Gets Here

Documentation graduates from `wip/` sessions:

1. **Storm Sessions** → Project documentation (e.g., `docs/homelab/`)
2. **Troubleshoot Sessions** → Remediation docs (`docs/troubleshooting/`) and guides (`docs/guides/`)

See `workstyle/working_style.md` for the full graduation workflow.

## Current Projects

| Project | Description | Status |
|---------|-------------|--------|
| [Homelab](homelab/) | Infrastructure architecture for greenfield homelab rebuild | Architecture complete |
