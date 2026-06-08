# Context: Local AI Assistant

**Goal**: Plan and design a locally hosted, sandboxed AI assistant deployed via Docker.
**Current State**: 
- **Architecture Pivot**: Adopted a **Dual-Agent Architecture**.
  1. **OpenCode Web Server**: Dedicated to coding and repository maintenance. Uses cloud frontier models. Mounts a shared `/repos` directory for easy access to multiple projects.
  2. **Hermes Assistant**: A persistent service (likely Discord-integrated) for general interactions, brainstorming, and specifically creating documentation/research.
- **Decision made**: Use cloud models (APIs) for heavy lifting (ADR drafted), though Hermes may run locally or via a cheaper cloud endpoint depending on hardware.
**Open Questions**:
- **Hermes as a Research Bot**: How will Hermes ingest data, store research, and be triggered?
- **Context Sharing**: How do we pass context (like a drafted document) from Hermes to OpenCode?
**Key Constraints**: 
- Must be containerized (Docker).
- OpenCode must securely sandbox code execution while accessing `/repos`.