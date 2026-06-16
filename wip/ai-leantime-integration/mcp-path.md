# MCP Path: OpenClaw ↔ Leantime

## Architecture

```
[Discord/Telegram/WhatsApp/Signal/etc.]
        │
        ▼
┌─────────────────────┐
│   OpenClaw Gateway  │  ← MCP client (built-in)
│  (openclaw mcp add) │
└─────────┬───────────┘
          │ MCP protocol (stdio or HTTP)
          ▼
┌─────────────────────┐
│  Leantime MCP       │  ← leantime-mcp bridge (npm package)
│  Bridge / Plugin    │
└─────────┬───────────┘
          │
          ▼
┌─────────────────────┐
│  Leantime Instance  │  ← self-hosted, MCP Server plugin enabled
│  (PHP + MySQL)      │
└─────────────────────┘
```

No n8n, no Zapier, no middleware. OpenClaw agent talks directly to Leantime via MCP tools.

## Prerequisites

| Item | Cost | Notes |
|------|------|-------|
| Leantime self-hosted (3.x+) | Free | Already planned for homelab |
| MCP Server plugin | Paid | Marketplace.leantime.io |
| Advanced Auth plugin | Paid | Needed for personal access tokens (bundle available) |
| leantime-mcp bridge | Free | `npm install -g leantime-mcp` |
| OpenClaw | Free | `openclaw onboard` or Docker |

## Setup

### 1. Leantime Plugins
1. Settings → Plugins → Marketplace
2. Install MCP Server plugin (enter license key)
3. Install Advanced Auth plugin (if not bundled)
4. Enable both
5. Verify `/mcp` endpoint is live at `https://leantime.your-domain/mcp`

### 2. Generate Token
1. Profile → Personal Access Tokens → Generate New Token
2. Copy the token (shown once)

### 3. Install MCP Bridge
```bash
npm install -g leantime-mcp
```

### 4. Register in OpenClaw

Via stdio tunnel (Leantime host has the bridge installed):

```bash
openclaw mcp add leantime \
  --command leantime-mcp \
  --arg https://leantime.your-domain/mcp \
  --arg --token \
  --arg YOUR_TOKEN
```

Or via OpenClaw JSON config (`openclaw.json`):

```json5
{
  "mcp": {
    "servers": {
      "leantime": {
        "command": "leantime-mcp",
        "args": [
          "https://leantime.your-domain/mcp",
          "--token",
          "YOUR_TOKEN"
        ]
      }
    }
  }
}
```

### 5. Verify
```bash
openclaw mcp doctor leantime --probe
```

## What the Agent Can Do

Once connected, OpenClaw agents gain MCP tools for Leantime. Example prompts from any channel:

- *"What are my open tasks in Leantime?"*
- *"Create a new task in the Homelab project: 'Research Tailscale subnet routing'"*
- *"Show me the milestones for Q3"*
- *"Move task 42 to In Progress"*
- *"What's the status of the Infrastructure project?"*

## Caveats

- **MCP plugin is paid** — not expensive, but not free. Evaluate if the JSON-RPC API alone would suffice first.
- **Personal Access Token** (Advanced Auth plugin) is recommended over API keys because API keys act as service accounts and can't answer "my tasks" queries.
- **Network**: OpenClaw must reach Leantime's MCP endpoint. If on different hosts, Tailscale or a LAN route needed.
- **Bridge location**: `leantime-mcp` bridge must run on a machine that can reach both the AI client and the Leantime instance. Can be same host as OpenClaw.
- **Leantime MCP tools are read+write** by default — consider whether the OpenClaw agent should have full CRUD or if tool filters should restrict destructive operations.
