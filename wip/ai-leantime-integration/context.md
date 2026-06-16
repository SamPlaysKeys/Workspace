# Context: AI + Leantime Integration

**Goal**: Explore using OpenClaw (multi-channel AI assistant gateway) with Leantime's MCP server/API for dynamic project and task management from anywhere.

**Current State**: MCP path confirmed viable. Details documented in `mcp-path.md`.

**Why OpenClaw**:
- Multi-channel inbox (Discord, Telegram, WhatsApp, Signal, iMessage, etc.)
- Built-in MCP client support (`openclaw mcp add/set`)
- Skills system for extensibility
- Sandboxed execution
- Local-first, self-hosted
- Multi-agent routing

**Leantime MCP Requirements**:
- MCP Server plugin from Leantime Marketplace (paid)
- Advanced Auth plugin for personal access tokens (or use API keys — but tokens needed for user-specific queries like "get my tasks")
- Node.js 18+ for the bridge
- MCP endpoint at `https://leantime-instance/mcp`

**OpenClaw MCP Client**:
- Register via `openclaw mcp add --command leantime-mcp --arg ...` or config
- Supports stdio and HTTP transports
- MCP tools projected into agent sessions automatically

**Open Questions**:
- Which Leantime features should be exposed? (tasks, projects, milestones, time tracking, goals?)
- Where does OpenClaw run? (same Docker host as Leantime, or separate VPS?)
- Do we need an n8n bridge for anything, or does MCP cover all use cases?
- How granular should tool access be? (read-only vs. create/update/delete)

**Key Constraints**:
- Leantime is self-hosted
- MCP plugin requires purchase
- OpenClaw must be able to reach Leantime's MCP endpoint (LAN or Tailscale)
