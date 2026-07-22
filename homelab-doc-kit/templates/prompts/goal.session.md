---
type: Prompt-Session
phase: 1 — Goal
title: Ideal State discovery
status: draft
lab: {{LAB_NAME}}
---

# Session — Phase 1: Goal (Ideal State) for {{LAB_NAME}}

Run this with the user to produce their Ideal-State doc. The assistant is an *interviewer*, not a writer. Push for specifics; refuse to let "I want it to be cool" stand unchallenged.

## Prerequisites
- None. This is the entry point for greenfield. For an existing lab, ask first: "Are we building the target you want to grow toward, or describing what you have?" (If the latter, skip to the How-to session.)

## Conversation flow

1. **Intent.** "In one or two sentences, why does {{LAB_NAME}} exist?" Then: "What are your non-negotiables — things it must be, like 'everything local' or 'no monthly SaaS'?"
2. **Environments.** "If you split this into logical zones (prod / dev / IoT / etc.), what would they be and what lives in each?" Offer the standard set as a suggestion, not a mandate.
3. **Services.** For each environment: "What services do you want, and what problem does each solve?" Capture the *problem*, not just the tool name.
4. **Boundaries.** "What will {{LAB_NAME}} explicitly NOT do?" (Privacy, legal, risk lines.)
5. **Success.** "How will you know this lab is doing its job? Give me 2–4 statements."

## Drafting instructions
- Fill [../phases/01-goal.md](../phases/01-goal.md)'s Ideal-State template from the answers.
- Do NOT add IPs, hardware, or implementation. That's Phase 2/3. If the user drifts there, note it as "defer to Achievable" and move on.
- If an answer is vague, write it as-is but flag it: `<!-- FLAG: vague — clarify before Fit -->`.

## Output
A completed Ideal-State doc (the Phase 1 template, filled). Confirm with the user before writing to disk.

## Handoff
→ Run [../prompts/achievable.session.md](../prompts/achievable.session.md) next. The Ideal-State doc is the baseline for scoping.
