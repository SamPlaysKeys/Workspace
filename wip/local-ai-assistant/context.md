# Context: Local AI Assistant

**Goal**: Plan and design a locally hosted, sandboxed AI assistant deployed via Docker.
**Current State**: 
- **Architecture**: **Tri-Layer Agent Architecture**
  1. **Nemoclaw Gateway** — Multi-channel ingress layer (Discord, iMessage, Telegram, etc.). Routes messages to the appropriate backend agent. Built from OpenClaw source with NVIDIA GPU passthrough. Deploys on Fedora VM with Quadro P5000 — config from `/DATA/AppData/Nemoclaw/openclaw`, workspace from `/DATA/AppData/Nemoclaw/workspace`.
  2. **OpenCode Web Server** — Dedicated to coding and repository maintenance. Uses cloud frontier models. Mounts a shared `/repos` directory for easy access to multiple projects.
  3. **Hermes Assistant** — A persistent service for general interactions, brainstorming, and documentation/research.
- **Decision made**: Use cloud models (APIs) for heavy lifting (ADR drafted), though Hermes may run locally or via a cheaper cloud endpoint depending on hardware.
**Open Questions**:
- **Hermes as a Research Bot**: How will Hermes ingest data, store research, and be triggered?
- **Context Sharing**: How do we pass context (like a drafted document) from Hermes to OpenCode?
**Key Constraints**: 
- Must be containerized (Docker).
- OpenCode must securely sandbox code execution while accessing `/repos`.