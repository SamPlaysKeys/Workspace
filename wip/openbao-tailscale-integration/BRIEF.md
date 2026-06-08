# OpenBao Tailscale Plugin — WIP Brief

> **Status:** Ready to Graduate
> **Last Consolidated:** 2026-06-07
> **Repository:** https://github.com/SamPlaysKeys/openbao-plugin-secrets-tailscale

## Abstract & Definition

A custom OpenBao secrets engine plugin that dynamically generates short-lived, preauthorized, and ephemeral Tailscale node authentication keys on-demand. When a client queries OpenBao at `docker/tailscale/auth-token/<service-name>`, the plugin calls the Tailscale API to create a node auth key, using the service name as the key description for tracking.

## Scope Boundaries

**In Scope:**
- OpenBao secrets engine plugin (Go) — register, configure, read auth tokens
- Tailscale API integration (key creation, not revocation)
- Docker multi-arch build (linux amd64/arm64, darwin amd64/arm64)
- Runtime templates: inline (app container), sidecar (Compose), operator (Kubernetes)

**Out of Scope:**
- Key revocation (no `Revoke` path — Tailscale handles lifecycle)
- Tailscale IDP/OIDC auth for OpenBao (separate wip session at `wip/tailscale-idp-openbao/`)
- Lease/TTL management in OpenBao (keys expire server-side via `expiry_seconds`)

## Files Catalog

### Plugin Source
- `main.go` — OpenBao plugin server entrypoint
- `backend.go` — Backend setup, path registration, seal-wrap config
- `path_config.go` — Config CRUD (API key, tailnet, base URL)
- `path_auth_token.go` — Core handler: builds Tailscale API request, returns auth key
- `go.mod` / `go.sum` — Go module + dependency lockfile

### Build
- `Dockerfile` — Multi-arch cross-compile builder (golang:1.24-alpine)
- `README.md` — Full documentation: install, configure, usage, POC steps
- `BRIEF.md` — This file

### Deployment Templates
- `templates/template.dockerfile` — Inline entrypoint pattern (Tailscale in app container)
- `templates/template.sidecar.Dockerfile` — Standalone sidecar image
- `templates/template.sidecar.docker-compose.yml` — Sidecar + app wiring
- `templates/template.docker-compose.yml` — Basic compose example
- `templates/template.operator.kubernetes.yml` — K8s sidecar + Tailscale Operator annotations
