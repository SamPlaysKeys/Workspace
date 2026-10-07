---
name: scrub
description: Security sanitization skill. Interactively redacts secrets, infra identifiers, and org metadata before graduating or committing files.
---

# Scrub — Interactive Sanitization

<objective>
Scan and interactively redact sensitive data (secrets, infrastructure identifiers, and org/user identity) before files are committed or graduated. Never edit files without user confirmation. Never use git commands like "add" or "commit" when dealing with original data.
</objective>

## Triggers
- "scrub [file/dir]", "sanitize [file/artifact]", "prepare for public/artifacts"
- Targets: "scrub secrets", "scrub infra", "scrub ident", or default to all three.

## Targets & Replacements
1. `secrets`: API keys, tokens, passwords, private keys -> `<REDACTED_API_KEY>`, `<REDACTED_PASSWORD>`
2. `infra`: Internal domains, RFC 1918 IPs, cluster names -> `*.example.com`, `192.0.2.x`, `<INTERNAL_IP>`
3. `ident`: Company/client names, codenames, emails, usernames -> `ExampleCorp`, `user@example.com`, `<USER>`

## Process
1. **Scope**: Identify requested targets (`secrets`, `infra`, `ident`, or all 3).
2. **Scan**: Inspect target file(s) and collect matches with line numbers.
3. **Propose**: Output a remediation table:
   | Line | Target | Detected (Masked) | Proposed Replacement | Reason |
4. **Alignment Check**: Ask: *"Does this match what you had in mind, or would you like to adjust any replacements?"*
5. **Apply & Verify**: After explicit approval, apply replacements and verify file syntax.
