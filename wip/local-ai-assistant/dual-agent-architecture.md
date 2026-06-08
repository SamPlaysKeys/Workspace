# Dual-Agent Architecture: OpenCode + Hermes

## Overview
Instead of forcing a single AI agent to be both a rigorous software engineer and a conversational brainstorming partner, we are splitting the responsibilities into two distinct, containerized services. 

### 1. The Builder: OpenCode Web Server
*   **Purpose**: Repository maintenance, coding, debugging, and git operations.
*   **Interface**: OpenCode WebUI (accessed via browser).
*   **Brain**: Cloud Frontier Models (e.g., Claude 3.5 Sonnet, GPT-4o) via API.
*   **Environment**: Runs in a secure Docker container with a shared host directory (e.g., `/workspace/repos/`) mounted into it. This allows the user to easily drop new repositories into the folder on the host, and OpenCode immediately has access to them.

### 2. The Thinker: Hermes Assistant
*   **Purpose**: General interactions, brainstorming, drafting documentation, and acting as a research assistant.
*   **Interface**: Discord Bot (for on-the-go access) and/or a lightweight chat UI (like LibreChat).
*   **Brain**: Hermes (either running locally via Ollama if hardware permits, or via a fast/cheap cloud API like Together AI).
*   **Environment**: A persistent Docker container running the bot logic, connected to a vector database (for memory/RAG) and web-search tools.

---

## System Layout

```text
HOST MACHINE
│
├── /workspace/repos/ (Shared Volume)
│   ├── project-a/
│   └── project-b/
│
├── [ Container 1: OpenCode Web ] ◄─────────► Browser (Deep Work)
│   ├── Mounts: /workspace/repos/
│   └── Tools: Bash, Git, File Edit, Read
│
└── [ Container 2: Hermes Bot ] ◄───────────► Discord / Chat UI (Ideation)
    ├── Mounts: /workspace/docs/ (Optional)
    └── Tools: Web Search, Markdown Generation
```

---

## Daily Interactions (A Day in the Life)

**Morning (Ideation & Research):**
You are having coffee and open Discord on your phone. You ping the Hermes bot: *"I want to add a new authentication flow to Project A using OAuth2. Can you research the best practices for this and draft an architecture document?"*
Hermes searches the web, synthesizes the information, and generates a markdown document. You ask it to save this to your shared `docs/` folder.

**Mid-Day (Execution):**
You sit at your desk and open the OpenCode WebUI in your browser. You point it to `/repos/project-a/` and say: *"Read the `auth-architecture.md` file Hermes just generated, and implement the OAuth2 flow as described."*
OpenCode reads the file, writes the code, runs the tests, and creates a git commit.

**Afternoon (Review & Tracking):**
You jump back into Discord and tell Hermes: *"OpenCode just finished the OAuth2 implementation. Can you update my personal tracking log to reflect that this is done, and draft a quick changelog for the users?"*

---

## Benefits
1.  **Separation of Concerns**: OpenCode is highly tuned for file operations but can be rigid for casual chat. Hermes is highly conversational and great at synthesis.
2.  **Cost Optimization**: You only pay for expensive frontier models when actually writing code. General chat and research can be routed through cheaper (or free/local) models.
3.  **Safety**: The Hermes bot, which might be exposed to Discord or the web, doesn't have bash access to your code repositories. Only the locally-accessed OpenCode container does.

## Challenges
1.  **Context Fragmentation**: OpenCode and Hermes don't share a brain. If you brainstorm with Hermes, you have to explicitly pass that context (usually via a written markdown file) to OpenCode.
2.  **Resource Management**: Running two separate stacks (even if lightweight) requires managing two sets of Docker Compose files, environment variables, and updates.