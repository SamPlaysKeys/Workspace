---
name: scrub
description: Security and privacy sanitization skill. Scans and interactively redacts secrets, internal infrastructure identifiers, and organization/personal metadata from files before graduation or public distribution.
---

# Scrub

<objective>
Sanitize files, directories, or diffs by identifying and interactively redacting sensitive information across three distinct target categories: secrets, infrastructure identifiers, and organizational/identity metadata. Always follow the Interactive Remediation pattern—never modify files silently or destructively without human review and confirmation.
</objective>

## Triggers
- User says "scrub this", "scrub the artifact", "sanitize this file/directory"
- User specifies targets: "scrub for secrets", "scrub infra and ident", "scrub all"
- Pre-graduation sanity check when promoting material from `wip/` to `artifacts/` or `docs/`

---

## Target Categories

The user may target one or any combination of the following categories. If unspecified, default to **all three**:

1. **`secrets` (Credentials & Sensitive Tokens)**
   - API keys, OAuth/bearer tokens, PATs, cloud credentials (AWS, GCP, Azure)
   - Passwords, hashes, secret connection strings
   - Private SSH/TLS keys and certificates
   - Sensitive webhook URLs and query params containing secrets

2. **`infra` (Infrastructure Identifiers)**
   - Internal FQDNs and domains (`*.corp.internal`, `*.local`, `*.internal.net`)
   - Private IP addresses (RFC 1918: `10.0.0.0/8`, `172.16.0.0/12`, `192.168.0.0/16`)
   - Specific internal cluster, host, namespace, or node naming schemes
   - Internal/private container registries, artifactory endpoints, proxy URLs

3. **`ident` (Organizational & Personal Metadata)**
   - Client, customer, and internal company names
   - Proprietary project codenames
   - Real employee names, LDAP handles, personal email addresses
   - Internal tenant IDs, billing account numbers, contract identifiers

---

## Standard Replacement Conventions

When proposing replacements, use clear, standardized placeholders that preserve code/syntax validity:

| Category | Typical Pattern | Standard Replacement Convention |
| :--- | :--- | :--- |
| **Secrets** | Passwords, API tokens | `<REDACTED_API_KEY>`, `<REDACTED_PASSWORD>`, `your-secret-token` |
| **Infra (Domains)** | `api.internal.mycompany.com` | `api.example.com` or `cluster.example.com` |
| **Infra (IPs)** | `10.240.12.4`, `192.168.1.50` | `192.0.2.10` (RFC 5737 TEST-NET), `10.0.0.X`, or `<INTERNAL_IP>` |
| **Infra (Registry)**| `registry.corp.net/internal` | `registry.example.com/repo` or `<CONTAINER_REGISTRY>` |
| **Identity (Company)**| `Acme Corp`, `MyClient Inc` | `ExampleCorp` or `<CUSTOMER_NAME>` |
| **Identity (Email)**| `john.doe@company.com` | `user@example.com` |
| **Identity (Names)**| Real person names | `<USER>` or `User` |

---

## Interactive Remediation Process

1. **Clarify Targets & Scope**
   - Check what targets the user requested (`secrets`, `infra`, `ident`, or `all`).
   - If not specified, state that you are scanning for all three categories.

2. **Scan & Detect**
   - Read and analyze the target file(s) or diff.
   - Collect every occurrence of matching strings with line numbers.

3. **Present Remediation Proposal**
   - Present findings grouped by category in an **Interactive Remediation Table**:
   ```markdown
   ## Scrub Proposal: [File or Directory]
   **Active Targets**: [secrets | infra | ident | all]

   | Line | Category | Detected Value (Masked) | Proposed Replacement | Notes / Context |
   | :--- | :--- | :--- | :--- | :--- |
   | 24 | secrets | `ghp_3a...9F` | `<REDACTED_GITHUB_TOKEN>` | GitHub Personal Access Token |
   | 42 | infra | `10.12.4.55` | `192.0.2.55` | RFC 1918 internal IP |
   | 58 | ident | `AcmeHealthCorp` | `ExampleCorp` | Client name |
   ```

4. **Alignment Checkpoint**
   - Ask: *"Does this match what you had in mind, or would you like to adjust any replacements before I apply them?"*
   - Wait for explicit user confirmation.

5. **Apply & Verify**
   - Once approved, apply replacements cleanly.
   - Perform a quick sanity read/diff to ensure formatting and syntax remain intact without lingering or partially-scrubbed tokens.
   - Report final completion summary.
