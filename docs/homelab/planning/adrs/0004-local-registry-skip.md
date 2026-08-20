---
type: ADR
Status: Accepted
Date: 2026-08-19
---

# ADR: Skip local Docker registry, focus on git pipeline

## Context

The homelab needs a strategy for managing container images between the local environment and production. Initial considerations included running a local container registry (Artifactory/Lightwell analog) or relying on Docker Hub with version pinning. The homelab already uses Komodo GitOps, GitHub Actions for rendering, and Ansible validation playbooks.

Goals:

1. **Avoid infrastructure overhead** — A local registry adds a running service, storage management, and maintenance burden for a feature (offline access) that isn't actually required.
2. **Leverage existing pipeline investment** — GitHub Actions, Ansible validation, and Komodo ResourceSync are already in place and can be extended to cover image governance.
3. **Security evaluation through PR process** — Image scanning and review can happen in Git before anything gets deployed, rather than after images sit in a registry.
4. **Version pinning is already the pattern** — Komodo configs and Git branches already provide effective version control; a registry would be redundant.

## Decision

**Skip running a local Docker registry as a stop-gap.** Instead:

1. **Pin images explicitly** in Komodo TOML configs (e.g., `nginx:1.25-alpine` instead of `latest`) — this is the primary guardrail.
2. **Extend the existing Git pipeline** to include:
   - Image scan on PR (Trivy or Anchore)
   - Push approved images to Docker Hub / existing registry with pinned tags
   - Komodo picks up the new commit SHA / tag on sync
3. **Use Docker Hub rate-limit-friendly practices** — pull once, cache locally via Docker's built-in store, and pin versions.
4. **Revisit local registry later** only if offline access or rate limits become actual problems.

## Considered alternatives

| Approach | Security Evaluation | Version Pinning | Offline | Maintenance |
|---|---|---|---|---|
| **Local Docker registry** | Manual (ad-hoc scanning) | ✅ Config-based | ✅ Yes | Low-Medium |
| **Docker Hub + pinning** | Manual (ad-hoc scanning) | ✅ Config-based | ❌ No | Low |
| **Git pipeline + image scanning** | ✅ Automated (PR gate) | ✅ PR-approved tag | ✅ CI cache | Medium |
| **Artifactory/Lightwell** | ✅ Scanning + promotion | ✅ Pinning | ✅ Yes | High |

## Consequences

**Positive**

- No extra running service or storage to manage
- Security gates move left — images are scanned and reviewed in PR before any deployment
- Existing Git/GitHub Actions investment leveraged; no new infra needed
- Version pinning already the pattern via Komodo configs; registry would be redundant
- Simpler onboarding for new services — just add to pipeline, no registry config needed

**Negative / risks**

- Image scanning depends on CI runner availability and speed
- If Docker Hub rate limits become problematic, no local cache (but this hasn't been an issue in practice)
- Security evaluation relies on PR review quality — same as any code change

**Mitigations**

- Add Trivy scan to existing GitHub Action as a checklist step — fail PR if critical vulnerabilities found
- Cache scanned results in GitHub Actions artifact so subsequent scans are fast
- If rate limits bite, add a registry later as a targeted fix (not infrastructure upfront)
- Document the "pinning is the guardrail" principle so teams don't revert to `latest`

## Status

**Accepted** — proceed with extending the Git pipeline for image governance instead of running a local registry.

## References

- Decisions: [decisions.md](../decisions.md#komodo-over-kubernetes-k3s-argocd)
- Komodo GitOps: [Komodo ResourceSync docs](https://komo.do/docs/automate/sync-resources)
- Existing ADR: [0001-komodo-resourcesync-branch-per-environment.md](0001-komodo-resourcesync-branch-per-environment.md)
- Trivy scanning: `trivy image-scan --exit-code 1 --severity HIGH,CRITICAL`