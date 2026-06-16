# Inconsistencies — Local AI Assistant Plan vs. Gemini Chat (2026-06-16)

- [ ] **Hermes execution scope**: Current plan restricts Hermes to research/docs — no bash or API execution. Gemini chat reveals Hermes should directly call REST APIs (Leantime) and control Home Assistant. Decide: does Hermes get sandboxed execution?
- [ ] **MCP server integration**: Leantime has an official MCP plugin/bridge (self-hosted). Current plan only mentions file-based context sharing. Decide: should MCP be the agent-to-tool integration mechanism?
- [ ] **Home Assistant use case**: Not in current plan. Gemini chat shows user wants agent-HA interaction, with Hermes recommended over OpenCode. Decide: add as integration target?
- [ ] **OpenCode as transitional**: Current dual-agent docs treat OpenCode and Hermes as permanent co-equal agents. User asked about starting with OpenCode and migrating to Hermes later. Decide: permanent split or on-ramp architecture?
- [ ] **n8n elimination**: User wants agents calling APIs directly without middleware. Affects tool design (API keys, HTTP capabilities, auth). Decide: does the architecture include n8n or skip it?
- [ ] **Hermes model tier**: Current plan says "cheap cloud API." Home Assistant (real-time state) and API execution may need a more capable model. Decide: single model tier or tiered routing per task type?
