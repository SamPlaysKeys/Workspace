---
source: https://github.com/leantime/leantime-mcp/blob/main/README.md
fetched: 2026-06-16
---

# Leantime MCP Bridge — README Summary

_Saved reference from the leantime-mcp npm package repository._

## Features

- Built with official `@modelcontextprotocol/sdk`
- Multiple auth methods: Bearer, API Key, Token, X-API-Key
- Protocol versions: MCP 2025-03-26 (latest), fallback 2024-11-05
- Transports: HTTPS, SSE, streaming
- TypeScript, type-safe

## CLI Usage

```
leantime-mcp <url> --token <token> [options]
```

### Parameters
- `<url>` — Leantime MCP endpoint (required)
- `--token <token>` — Auth token (required)
- `--auth-method <method>` — Bearer (default), X-API-Key
- `--insecure` — Skip SSL verification (dev only)
- `--max-retries <num>` — Default 3
- `--retry-delay <ms>` — Default 1000
- `--no-cache` — Disable response caching

### Client Config Examples

**Claude Desktop:**
```json
{
  "mcpServers": {
    "leantime": {
      "command": "leantime-mcp",
      "args": ["https://yourworkspace.leantime.io/mcp", "--token", "YOUR_TOKEN"]
    }
  }
}
```

**Claude Code:**
```json
{
  "mcp": {
    "servers": {
      "leantime": {
        "command": "leantime-mcp",
        "args": ["https://yourworkspace.leantime.io/mcp", "--token", "YOUR_TOKEN"]
      }
    }
  }
}
```

## Advanced Features

- **Retry logic**: Exponential backoff with jitter (±25%)
- **Smart caching**: tools/list, resources/list, prompts/list cached for 5 min TTL
- **Session management**: Tracks MCP session IDs for stateful interactions
- **Graceful shutdown**: Clean termination on SIGINT/SIGTERM

## Security Considerations

- HTTPS only in production
- Store tokens securely
- Only use `--insecure` for dev with self-signed certs
- Consider token rotation for long-running processes
