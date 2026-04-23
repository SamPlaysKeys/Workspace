# Troubleshoot Behavior (draft)

## Summary

**Invocation:** "Troubleshoot" / "Debug" / agent-recognizes problem context

**Trigger:** User-initiated, or agent-initiated when problem-solving context is detected (error messages, "this isn't working," "I'm having an issue with...").

Troubleshoot is a structured mode for investigating issues and capturing remediation knowledge. Unlike Storm Session (disposable brainstorming), Troubleshoot builds reference material that persists.

---

## What Troubleshoot Is For

- Investigating errors, failures, misconfigurations
- Working through problems with back-and-forth debugging
- Capturing what was tried and what worked
- Building a knowledge base of fixes and setup guidance

## What Troubleshoot Is NOT For

- Brainstorming new features or designs (use Storm Session)
- One-off questions with quick answers (just answer directly)
- Documenting known procedures (just write to docs/)

---

## Session Structure

Troubleshooting sessions live in `.wip/troubleshoot-<system>-<issue>/` during active investigation.

```
.wip/troubleshoot-openshift-storage-pvc-pending/
  investigation.md    # Running log — the working document
```

Lightweight by design. One file tracks the whole investigation.

**Naming:** `troubleshoot-<system>-<brief-issue>` — e.g., `troubleshoot-openshift-storage-pvc-pending`, `troubleshoot-docker-network-timeout`.

---

## investigation.md — The Running Log

This file is updated progressively as the investigation proceeds. Structure:

```markdown
# <System>: <Issue Summary>

**Status:** investigating | resolved | blocked | abandoned

**Environment:** [relevant versions, context]

---

## Symptoms

- What's happening?
- Error messages, unexpected behavior
- When did it start? What changed?

---

## Investigation

### <Timestamp or Step>

**Hypothesis:** What we think might be wrong

**Tried:** What we did

**Result:** What happened

---

(repeat as investigation proceeds)

---

## Resolution

**Root cause:** What was actually wrong

**Fix:** What resolved it

**Prevention:** How to avoid this in the future (if applicable)
```

Update this file *as you go* (Progressive Bookkeeping applies). Don't batch updates for the end.

---

## Graduation — Distributable Documentation

When the investigation resolves, the agent creates distributable documentation.

**Trigger:** "Create a walkthrough" / "Write this up" / agent prompts: "Want me to write this up?"

### Two Output Types

**1. Remediation Doc** (always created)
> "How to fix this issue"

Contains: Symptoms → Investigation → Resolution. For others hitting the same problem.

```markdown
# Fixing <Issue> on <System>

## Symptoms

What you'll see when this issue occurs...

## Investigation

How to diagnose this issue:

1. Check...
2. Look for...

## Resolution

Step-by-step fix:

1. ...
2. ...

## Verification

How to confirm it's fixed.
```

**2. Prevention / Setup Doc** (conditional)
> "How to set this up right the first time"

Contains guidance to avoid the issue entirely — correct setup, configuration, or practices.

```markdown
# Setting Up <Component> on <System>

## Overview

What this configures and why it matters.

## Prerequisites

- ...

## Configuration

Step-by-step setup:

1. ...
2. ...

## Common Pitfalls

What to avoid (learned from troubleshooting).

## Verification

How to confirm it's working.
```

### Graduation Decision Logic

| Agent assessment | Action |
|------------------|--------|
| Prevention doc clearly needed | Create both docs |
| Unsure or probably not needed | Prompt user: "Should I also write a prevention/setup guide for this?" |
| User says no | Skip prevention doc |

**Important:** The agent never unilaterally decides to skip the prevention doc. If in doubt, ask. User has final say.

### Where Distributables Live

```
docs/
  troubleshooting/
    <system>/
      <issue>.md           # Remediation docs
  guides/
    <system>/
      <component>.md       # Prevention / setup docs
```

---

## Lifecycle

### Full Troubleshooting Session

1. **Detect/Invoke:** Recognize troubleshooting context or explicit trigger
2. **Start session:** Create `.wip/troubleshoot-<system>-<issue>/investigation.md`
3. **Investigate:** Update investigation.md progressively
4. **Resolve:** Document root cause and fix in investigation.md
5. **Graduate:** 
   - Prompt to write up docs
   - Create remediation doc (always)
   - Create prevention doc (if clearly needed) or prompt user (if unsure)
6. **Discard session:** Delete `.wip/` directory — investigation.md is disposable once docs exist

### Quick Doc Path

Not everything needs a full session. Use the quick path when:
- The fix is already known (no investigation needed)
- User asks to document something common ("write up how to configure X")
- Quick question-and-answer that's worth capturing

**Quick path flow:**
1. User asks for doc or describes known fix
2. Agent writes directly to `docs/troubleshooting/` or `docs/guides/`
3. No `.wip/` session needed

---

## Agent Behavior

**Entering Troubleshoot mode:**
When the user describes a problem (error, misconfiguration, unexpected behavior), the agent may suggest: "Want to open a troubleshooting session for this?"

**During investigation:**
- Keep investigation.md current
- Clearly state hypotheses before trying things
- Record what was tried and results, even dead ends

**At resolution:**
- Summarize root cause and fix
- Prompt: "Want me to write this up?"
- Create remediation doc (always)
- Assess whether prevention doc is needed:
  - If clearly yes → create it
  - If unsure or probably no → ask user
  - Never skip without user confirmation

---

## Resolved Questions

- **investigation.md graduation:** Disposable. Delete with session once docs exist.
- **Quick path:** Yes. Known fixes or doc requests skip the session, write directly to docs/.
- **Tags/categories for findability:** Deferred. Added to BACKLOG for future consideration.
