---
type: Reference
subcategory: Platform
category: Homelab
status: Active
date: 2026-09-03
---

# Leantime MCP Server — Local Setup

How the Leantime MCP server is set up and running on this machine (sams-macbook-air), for use as an opencode MCP tool.

## Overview

Leantime is a self-hosted project management tool running in the homelab. It exposes an MCP endpoint at `/mcp`. This machine runs the `leantime-mcp` npm bridge as a **stdio-to-HTTP proxy**: it speaks the MCP protocol over stdio to the client (opencode), then forwards every tool call as HTTP JSON-RPC to the remote Leantime endpoint using a personal access token.

## Architecture

```
[opencode / MCP client]
        │ MCP protocol (stdio)
        ▼
leantime-mcp bridge (local process, /opt/homebrew/bin/leantime-mcp)
        │ HTTP JSON-RPC + Bearer token
        ▼
Leantime instance  http://leantime.samplayskeys.ts.net/mcp
        (served from fedora-dev, reached via Tailscale)
```

No n8n, no Zapier, no reverse proxy — the agent talks to Leantime directly through MCP tools.

## Configuration

Registered in `~/.config/opencode/opencode.json` under the `mcp` block:

```json
"leantime": {
  "type": "local",
  "command": ["leantime-mcp"],
  "enabled": true,
  "environment": {
    "LEANTIME_SERVER_URL": "http://leantime.samplayskeys.ts.net/mcp",
    "LEANTIME_API_TOKEN": "tOInzazhMmw9gxFVQmSV1mYp7aykn0UbaJ9w4K3l"
  }
}
```

### Bridge binary

- `leantime-mcp` v1.6.5 is an npm global install (`/opt/homebrew/bin/leantime-mcp`).
- It is a **symlink to a local source checkout** at `~/Src/leantime-mcp` (upstream: `github.com/leantime/leantime-mcp`), built to `dist/` via `tsc`.
- Bin wrapper: `bin/leantime-mcp.js` imports `dist/index.js` and runs the server entrypoint.

### Config resolution (`src/config.ts`)

- No CLI args are passed from opencode.json; config comes from the environment variables.
- `LEANTIME_SERVER_URL` → server URL, `LEANTIME_API_TOKEN` → token.
- Defaults: `Bearer` auth, 3 retries with jittered exponential backoff, 5-minute tool cache.

### Proxy behavior (`src/proxy.ts`)

- Lazily initializes the Leantime session on first tool list (MCP `initialize` + `notifications/initialized`).
- Every client tool call is proxied to the remote `/mcp` endpoint with the token.
- Retries failures with backoff, caches tool responses for 5 minutes.

## Leantime server

- Self-hosted at `http://leantime.samplayskeys.ts.net/mcp`, reachable only over Tailscale.
- MagicDNS resolves the name to `100.88.126.2`; `tailscale ping` reports it is served from **fedora-dev** (`100.125.91.5`). No Tailscale Serve config exists on this Mac — serving happens on the host side.
- The `/mcp` endpoint is auth-gated: unauthenticated requests return 401/406; the root path 302-redirects. Requests must include `Authorization: Bearer <token>`.
- Requires the Leantime **MCP Server plugin** (paid, marketplace) for the endpoint and **Advanced Auth** for personal access tokens (token needed for user-specific queries like "get my tasks").

## History / context

Originally explored for OpenClaw in the WIP session `wip/ai-leantime-integration/` (see `context.md`, `mcp-path.md`, `refs/`). The bridge ended up wired into opencode on a local machine instead.

## Pitfalls / notes

- The bridge must run on a machine that can reach both the AI client and the Leantime instance (Tailscale overlay covers this).
- Leantime MCP tools are read+write by default — consider tool filters if destructive operations should be restricted.
- API keys act as service accounts and cannot answer user-specific queries; prefer a personal access token.
