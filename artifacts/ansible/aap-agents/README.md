---
type: README-Note
---

# AAP Agents — Ansible Automation Platform Tooling & Agent Skills

A modular toolkit designed for AI coding agents (and human engineers) to interact with, monitor, troubleshoot, and verify Ansible Automation Platform (AAP) / AWX controller jobs directly from the CLI.

---

## Directory Structure

```text
artifacts/ansible/aap-agents/
├── README.md                 # This guide
├── scripts/                  # CLI tools and Python helpers
│   ├── _aap_lib.py           # Shared library: credentials, HTTP, caching, security scanning
│   ├── aap-api               # Bash wrapper for arbitrary AAP REST API queries
│   ├── aap-failures.py       # Extract failure details and failed task events
│   ├── aap-poll.sh           # Non-blocking terminal polling loop with task heartbeats
│   ├── aap-runs.py           # List and filter recent job runs by template and branch
│   ├── aap-scan-stdout.py    # Download stdout and scan for error/status signals
│   ├── aap-setup             # Configure and validate AAP PAT credentials
│   ├── aap-stream.py         # Real-time stdout streamer (WebSocket with REST fallback)
│   ├── aap-workflow.py       # Inspect workflow job trees and child node statuses
│   └── requirements.txt      # Python dependencies (detect-secrets, websocket-client)
└── skills/                   # Agent Skills for interactive workflows
    ├── aap-live-monitor/     # Proactive validation during running jobs
    ├── aap-troubleshoot/     # Structured triage and root-cause analysis for failed jobs
    ├── aap-branch-review/    # Multi-run health rollups across a git branch
    ├── aap-verification-gate/# SCM-commit-aware PASS/BLOCK merge verdict
    └── decision-trail-update/# PR evidence generator combining git diffs + AAP test runs
```

---

## Authentication & Setup

The scripts read credentials either from local config files or standard environment variables.

### Option 1: Interactive Setup (Recommended for Local Dev)

Run the interactive setup helper:

```bash
./scripts/aap-setup
```

This creates `~/.config/aap/` with:
- `token`: Personal Access Token (PAT)
- `hostname`: AAP Controller hostname (e.g., `aap.example.com`)
- `aap-api`: Installed symlink/copy of the raw API query helper

To verify or refresh an existing token:
```bash
./scripts/aap-setup --check
./scripts/aap-setup --refresh-token
```

### Option 2: Environment Variables (CI/CD & Containers)

All scripts respect standard environment variables if set:
- `AAP_TOKEN`: AAP Personal Access Token
- `AAP_HOST` (or `AAP_HOSTNAME`): AAP instance hostname
- `AAP_INSECURE`: Set to `1` or `true` to disable SSL certificate verification for test/lab controllers

### Python Dependencies

```bash
pip install -r scripts/requirements.txt
```

---

## Scripts Reference

| Script | Purpose | Example Usage |
| :--- | :--- | :--- |
| `aap-setup` | Configures and validates credentials in `~/.config/aap/` | `./scripts/aap-setup --check` |
| `aap-api` | Zero-dependency Bash wrapper for arbitrary AAP endpoints | `./scripts/aap-api GET /api/v2/me/` |
| `aap-stream.py` | Real-time stdout streaming via WebSocket with REST fallback | `python3 scripts/aap-stream.py <job_id> --tail` |
| `aap-poll.sh` | Terminal heartbeat loop showing active task and elapsed time | `./scripts/aap-poll.sh <job_id> 15` |
| `aap-failures.py` | Pulls job metadata, failed task names, hosts, and error messages | `python3 scripts/aap-failures.py <job_id>` |
| `aap-scan-stdout.py`| Scans stdout for failures, retries, timeouts, and custom regex | `python3 scripts/aap-scan-stdout.py <job_id> --context 3` |
| `aap-runs.py` | Lists recent job runs by job template name and optional branch | `python3 scripts/aap-runs.py "Deploy Cluster" main` |
| `aap-workflow.py` | Inspects workflow node execution hierarchies and failed nodes | `python3 scripts/aap-workflow.py <workflow_job_id>` |

---

## Agent Skills Reference

These skills give AI coding agents explicit operational runbooks for managing AAP automation:

1. **`aap-live-monitor`**: Proactive supervision during execution. Catches semantic bugs (e.g., skipped plays, filter misconfigurations, empty loops) that Ansible might otherwise treat as `ok`.
2. **`aap-troubleshoot`**: Phased triage after a job failure. Separates infrastructure/network issues from playbook code errors and environment input bugs.
3. **`aap-branch-review`**: Aggregates multiple job runs across a branch to detect transient flakes vs. systematic code regressions.
4. **`aap-verification-gate`**: Evaluates whether the exact git commit at the tip of the PR was tested and succeeded in AAP before granting a merge PASS.
5. **`decision-trail-update`**: Automatically generates structured GitHub PR comments detailing recent commit themes, linked AAP job evidence, and validated vs. unexercised code paths.

---

## Built-In Security Scanning

`_aap_lib.py` and `aap-scan-stdout.py` integrate with `detect-secrets` alongside regex patterns for shell/Ansible credentials. Every stdout download automatically checks for leaked tokens, private keys, passwords, or AWS keys before writing to terminal output.
