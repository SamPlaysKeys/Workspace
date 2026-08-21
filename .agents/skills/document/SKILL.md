---
name: document
description: Generates standardized documentation (troubleshooting, guides, architecture) with consistent frontmatter.
---

# Document

<objective>
Create consistent, frontmatter-rich documentation. This skill is often called at the end of a `troubleshoot` or `storm session` to graduate findings into permanent reference material.
</objective>

## Triggers
- User asks to "write this up", "document this", or "create a guide".
- Handoff from the `troubleshoot` skill.

## Process

1. Determine the `type` of output requested (Troubleshooting vs. Guide vs. Reusable Artifact / Plan Template).
2. Extract the relevant context from the conversation history, **especially the primary external reference URL**.
3. Generate the output using the exact templates below.
4. Save to the appropriate directory:
   - Reference documentation -> `docs/troubleshooting/` or `docs/guides/`
   - Reusable automation templates, scripts, manifests, and AI execution plan templates -> `artifacts/<system>/` or `artifacts/templates/`

## Templates

### 1. Troubleshooting (Remediation)
**Use when**: Documenting a specific error, bug, or failure and how to fix it.
**Path**: `docs/troubleshooting/<system>/<issue-name>.md`

```markdown
---
type: Troubleshooting
status: Active
system: [System Name]
related_to: []
references: []
---

# Fixing [Issue Name] on [System]

## Symptoms
[What the user sees, exact error messages, stack traces]

## Root Cause
[Brief explanation of why it failed]

## Resolution
[Step-by-step fix]
1. ...
2. ...

## Verification
[How to confirm it's fixed]

## References
[Less is more. If a single authoritative resource covers the solution, only include that one.]
- [Link Title](URL) - [Brief note on what this link provided]
```

### 2. Guide (Prevention / Setup)
**Use when**: Documenting how to set something up correctly, or how to avoid an issue entirely.
**Path**: `docs/guides/<system>/<topic>.md`

```markdown
---
type: Guide
status: Active
system: [System Name]
related_to: []
references: []
---

# [Topic / Setup] for [System]

## Overview
[What this configures and why it matters]

## Prerequisites
- [Requirement 1]

## Configuration
[Step-by-step setup]
1. ...
2. ...

## Common Pitfalls
[What to avoid, learned from past troubleshooting]

## References
[Less is more. If a single authoritative resource covers the solution, only include that one.]
- [Link Title](URL) - [Brief note on what this link provided]
```