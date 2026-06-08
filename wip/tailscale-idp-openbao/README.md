# Tailscale IDP for OpenBao

Passwordless authentication for OpenBao using Tailscale identities — eliminating static tokens and secrets for both human operators and machine workloads.

## Concepts

Three integration paths were explored:

### 1. Human SSO via OpenBao OIDC + `tsidp`

OpenBao's native `jwt` auth method is configured with `tsidp` (Tailscale Identity Provider) as the upstream OIDC provider. A user runs `bao login -method=oidc`, gets redirected to `tsidp` for tailnet-based authentication, and receives an OpenBao token.

- Cleanest path for interactive human access (CLI & Web UI)
- `tsidp` serves a standard OIDC discovery document and JWKS endpoint
- Experimental STS token exchange (`TSIDP_ENABLE_STS=1`) extends this to non-interactive service-to-service scenarios (RFC 8693)

### 2. Machine/Container Auth via OpenBao JWT + `tsiam`

A `tsiam` daemon running on each Tailscale node signs node-identity JWTs (containing node name, tags, owner). Containers fetch these JWTs locally and present them to OpenBao's built-in JWT auth method (`bao write auth/jwt/login jwt=...`).

- No static secrets in containers — tokens are generated on demand from local tailnet identity
- Zero-plugin OpenBao setup — uses the native JWT auth engine

### 3. Passwordless Node Auth via Custom OpenBao Plugin + Tailscale WHOIS

A custom OpenBao credential plugin intercepts the client's IP from `logical.Request.Connection.RemoteAddr` and queries the local `tailscaled` socket's WHOIS API (`/localapi/v0/whois`) to verify identity and tags.

- Zero credentials on the client — the container simply sends a login request
- The plugin maps tailnet tags (e.g. `tag:prod-app`) to OpenBao policies

## Recommendation

| Audience | Approach |
|---|---|
| Human operators | `tsidp` + native OpenBao OIDC |
| Docker workloads (no plugin) | `tsiam` + native OpenBao JWT |
| Docker workloads (zero-credential client) | Custom WHOIS auth plugin |

## Status

Exploratory — findings captured but no implementation yet. See `discussion.md` for detailed architecture analysis and `context.md` for scope and open questions.
