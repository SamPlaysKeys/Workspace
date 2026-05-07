# Discussion

## 2026-05-07 — Session Start

Starting exploration of how to improve per-agent adoption of the working style.

Initial framing: The working style defines behaviors and conventions, but getting consistent adoption across different AI agents (Claude, GPT, etc.) and contexts (Cursor, API, other tools) is the challenge.

---

## 2026-05-07 — Direction: Rule Generation Over Context Injection

**Problem reframed:**
- Current approach: always-on rule summarizes behaviors → fills context window
- Pain point: agents still skip Progressive Bookkeeping despite the summary
- Hypothesis: agents build better habits from rules they generate themselves

**Proposed approach:**
- Minimal meta-instruction: "here's how to create working style rules"
- Agents read source (`workstyle/working_style.md`) only when needed
- Agents generate their own rule files for specific behaviors
- Rules become always-on after creation — learned, not loaded

**Open questions:**
- What's the minimal meta-rule that enables this?
- Should each behavior get its own rule file, or group them?
- How do we trigger rule creation? (first invocation? explicit command?)
- Does this actually improve Progressive Bookkeeping adoption?

---

## 2026-05-07 — Onboarding Document Draft

**Decision:** Create a separate onboarding document (not replacing existing working style). This doc instructs agents to create rules in their *native format*.

**Key insight:** Agent-agnostic means the agent decides the format:
- Cursor → `.cursor/rules/*.mdc`
- OpenCode → `.opencode/rules/*.md`
- Gemini → `.gemini/`
- etc.

**Draft created:** `scratch/onboarding.md`

**Design choices in draft:**
- Instructs agents to translate, not copy-paste
- Prioritizes Progressive Bookkeeping explicitly
- Includes self-check mechanism ("If session ended now...")
- Suggests creating a skill for on-demand rule generation

**Open for review:**
- Is the tone right? (instructing agent, not human)
- Should it live in `workstyle/` alongside the main doc?
- Do we need format-specific examples for each agent type?

---
