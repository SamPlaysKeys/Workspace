# Running Discussion

## 2026-06-03
Initialized storm session for planning a locally hosted AI assistant on Docker.
## 2026-06-03 (Update)
- Decided to pivot from local inference to cloud-based models (APIs) to eliminate heavy hardware requirements.
- Drafted ADR: `adr-cloud-vs-local-models.md` to capture the trade-offs (hardware versatility vs. data control and recurring costs).

## 2026-06-03 (Update 2)
- User clarified the interface and use-case requirements:
  - **Interfaces**: Wants multi-channel access. Discord for quick chats/ideas, and WebUIs (AIonUI or OpenCode Web) for deeper work.
  - **Use Cases**: Maintaining specific Git repositories (coding) + general ideation and tracking.
  - **Execution**: Needs to be sandboxed (mentioned NVIDIA OpenShell) to safely execute code.
  - **Agent Logic**: Debating between a standard model with a harness (OpenCode/Pi) vs. a specific assistant model (Hermes).
- **Architectural Implication**: This requires a decoupled, headless agent architecture. The core agent runs in a secure Docker sandbox, connects to cloud APIs for "brains", and exposes webhooks/websockets to Discord and WebUIs.

## 2026-06-03 (Update 3)
- User proposed a **Dual-Agent Architecture**:
  - **Container 1 (OpenCode Web)**: Focused purely on coding. Mounts a shared `repos/` folder. Uses the OpenCode harness for reliable file/git operations.
  - **Container 2 (Hermes)**: Focused on general interactions, documentation, and research. Likely accessed via Discord or a simple chat UI.
- Drafted `dual-agent-architecture.md` to detail the layout, benefits, challenges, and daily interactions.
- Next phase: Define the requirements for turning Hermes into a dedicated "Research Bot".
