---
type: README-Note
---

# Security — Sanitization & Hardening Artifacts

A collection of security-focused tools, agent skills, and validation patterns for identifying and remediating sensitive data, secrets, internal infrastructure markers, and credentials across repositories and public artifacts.

---

## Directory Structure

```text
artifacts/security/
├── README.md         # This guide
└── skills/
    └── scrub/        # Interactive sanitization skill for secrets, infra, and identity
```

---

## Skills Overview

### `scrub`
- **Objective:** Interactively scan and redact sensitive data (secrets, infrastructure identifiers, and org/user identity) before files are committed, PR'd, or graduated to public artifacts.
- **Workflow:**
  1. Identifies requested targets (`secrets`, `infra`, `ident`, or default to all three).
  2. Scans files and collects line-by-line sensitive matches.
  3. Proposes an explicit remediation table using standard RFC/generic placeholders.
  4. Prompts human reviewer for alignment approval before modifying any files.
  5. Applies approved replacements and verifies syntax.
- **Triggers:** "scrub [file/dir]", "sanitize [file/artifact]", "prepare for public/artifacts".
- **Target Categories:**
  - `secrets`: API keys, tokens, passwords, private keys &rarr; `<REDACTED_API_KEY>`, `<REDACTED_PASSWORD>`
  - `infra`: Internal domains, RFC 1918 IPs, cluster names &rarr; `*.example.com`, `192.0.2.x`, `<INTERNAL_IP>`
  - `ident`: Company/client names, codenames, emails, usernames &rarr; `ExampleCorp`, `user@example.com`, `<USER>`
