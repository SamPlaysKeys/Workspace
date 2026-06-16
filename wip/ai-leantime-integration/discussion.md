# Running Discussion

## 2026-06-16
Session started. Exploring OpenClaw + Leantime integration for dynamic project/task management from anywhere.

**Correction**: The Gemini transcription mangled "OpenClaw" as "Open Cloth"/"Open Claw." It's OpenClaw (https://github.com/openclaw/openclaw) — a multi-channel AI assistant gateway with MCP support, skills, sandboxing, and 20+ chat integrations.

## The MCP Path
Leantime has an official MCP plugin/bridge for self-hosted instances. OpenClaw has built-in MCP client support. This means wiring them together could be as straightforward as:
1. Install Leantime MCP plugin on the Leantime instance
2. Configure OpenClaw with the MCP server endpoint
3. OpenClaw agents can then read/write projects, tasks, milestones via natural language

This skips n8n/Zapier entirely — agents talk directly to Leantime.

## MCP Path Detailed
Created `mcp-path.md` with full architecture diagram, prerequisites table, step-by-step setup, and caveats.

## Feature Wishlist
Created `feature-wishlist.md` with 6 tiers of Leantime features ranked by value for AI interaction.

## Reference Docs Added
- `refs/leantime-mcp-guide.md` — Official Leantime MCP Server guide (setup, tools list, use cases)
- `refs/leantime-mcp-bridge-readme.md` — leantime-mcp npm package README (CLI flags, client configs, security)
