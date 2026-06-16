---
source: https://support.leantime.io/en/article/leantimes-mcp-server-guide-workflows-use-cases-191njhh/
fetched: 2026-06-16
---

# Leantime MCP Server Guide — Workflows & Use Cases

_Note: This is a saved reference copy of the official Leantime MCP Server guide._

## What is MCP?

MCP (Model Context Protocol) is a standardized way for AI assistants to interact with external tools and data sources.

## Prerequisites

- Leantime 3.x or later (self-hosted)
- MCP Server plugin from the Leantime Marketplace
- Node.js 18+ (for the MCP bridge)
- An MCP-compatible AI client

## Installation Steps

### Step 1: Install the MCP Server Plugin
1. Settings → Plugins → Marketplace
2. Find and purchase the "MCP Server" plugin
3. Enter license key, install, enable
4. `/mcp` endpoint available at `https://your-leantime-url/mcp`

### Step 2: Install the MCP Bridge
```bash
npm install -g leantime-mcp
```

Alternative from source:
```bash
git clone https://github.com/leantime/leantime-mcp.git
cd leantime-mcp
npm install && npm run build && npm install -g .
```

### Step 3: Generate an Access Token
- **Option A (Recommended):** Personal Access Token — Profile → Personal Access Tokens
- **Option B:** Standard API Key — Settings → API (format: `lt_{username}_{hash}`)

Personal tokens are tied to user accounts so queries like "my tasks" work. API keys are service accounts and can't do user-specific queries.

## Available MCP Tools

### Projects
`getAllProjects`, `getProject`, `getFullProjectOverview`, `addProject`, `editProject`, `findProject`, `getUsersAssignedToProject`

### Tasks
`findTasks`, `getTicket`, `addTask`, `editTask`, `addSubtask`, `bulkAddTasks`, `bulkEditTasks`, `getStatusLabels`

### Milestones
`findMilestones`, `getMilestone`, `addMilestone`, `editMilestone`, `addMilestonesForProject`

### Goals (OKRs)
`getAllGoals`, `getGoal`, `createGoal`, `editGoal`, `createGoalboard`, `getGoalsByMilestone`

### Calendar & Scheduling
`getCalendar`, `addEvent`, `editEvent`, `deleteEvent`, `scheduleTaskOnCalendar`, `scheduleDay`, `breakdownTask`, `getICalUrl`

### Timesheets
`getUserTimesheets`, `getProjectTimesheets`, `logTime`, `getTimesheetSummary`, `getWeeklyTimesheets`

### Comments & Status
`getComments`, `addComment`, `getAllProjectComments`, `addProjectStatusUpdate`

### Timer
`startTimer`, `stopTimer`

## STDIO Mode (Direct Server Connection)
```json
{
  "mcpServers": {
    "leantime": {
      "command": "php",
      "args": [
        "/path-to-leantime/bin/leantime",
        "lt-mcp:start",
        "--transport=stdio",
        "--token=YOUR_TOKEN"
      ]
    }
  }
}
```

## Alternative: Community MCP Server (no plugin required)
Uses Leantime's JSON-RPC API directly via Python/uv:
```json
{
  "mcpServers": {
    "leantime": {
      "command": "uvx",
      "args": ["--from", "git+https://github.com/daniel-eder/leantime-mcp.git", "leantime-mcp"],
      "env": {
        "LEANTIME_URL": "https://your-leantime-instance.com",
        "LEANTIME_API_KEY": "your_api_key",
        "LEANTIME_USER_EMAIL": "your_email@example.com"
      }
    }
  }
}
```

## Key Security Settings (Environment Variables)
| Variable | Description |
|---|---|
| `MCP_SERVER_ENABLED=true` | Enable MCP server |
| `MCP_REQUIRE_AUTH=true` | Require authentication |
| `MCP_REQUIRED_ROLE=editor` | Minimum role for access |
| `MCP_ALLOWED_IPS` | IP whitelist |
| `MCP_RATE_LIMIT=100` | Rate limiting |
| `MCP_CACHE_TOOLS=true` | Tool caching for performance |
