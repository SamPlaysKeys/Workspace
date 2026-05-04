# Working Style: Co-Creation Conventions

This document defines how AI agents should collaborate in this workspace. These conventions are agent and model agnostic — any AI assistant working here should follow them.

---

## Behaviors

Behaviors are defined modes of interaction that can be invoked by name. Some are user-initiated, some are agent-initiated, and some are always-on.

| Behavior | Invocation | Trigger |
|----------|------------|---------|
| **Storm Session** | "Storm Session" / "Storm Sesh" | User-initiated |
| **Pre-Mortem** | "Pre-Mortem" | User or agent (at decision points) |
| **Smooth Brain** | "Smooth Brain" | User or agent (at session start) |
| **Progressive Bookkeeping** | — | Always-on |
| **Dead Drop** | "Dead Drop" / "Bread Crumb" | User or agent (at session end) |
| **Troubleshoot** | "Troubleshoot" / "Debug" | User or agent (problem context detected) |
| **Sneaky** | "Sneaky" / "Hide your actions" | User-initiated |

---

### Storm Session

**Invocation**: "Storm Session" or "Storm Sesh"

Storm Sessions are focused brainstorming on net-new ideas. The `wip/` directory serves as a whiteboard for rapid ideation before committing to deeper work.

#### What Storm Sessions Are For

- Brainstorming and exploring new concepts
- Drafting abstracts, outlines, and initial structures
- Quickly fleshing out ideas before deeper research
- Collaborative ideation that benefits from back-and-forth

#### What Storm Sessions Are NOT For

- Editing or reworking established files (just edit those in place)
- Long-term project management
- Permanent documentation

#### Session Lifecycle

1. **Start**: Create a topic-based session in `wip/`
2. **Iterate**: Workshop the idea across one or more conversations
3. **Graduate**: Move valuable artifacts to their permanent homes
4. **Discard**: Delete the session — the brainstorming itself doesn't need to persist

Sessions are **disposable by design**. The output matters; the messy process doesn't need to be in version control.

#### Session Structure

```
wip/
  <topic-name>/
    context.md       # What we're exploring, current state, open questions
    discussion.md    # Running log of ideas and exchanges
    scratch/         # Draft files, snippets, experiments
```

**Naming**: Use the topic or concept name (`api-gateway-design`, `k8s-monitoring-strategy`, `blog-post-ci-cd`).

#### `context.md` — The Resumption File

This is the most important file for continuity. It should allow any agent to pick up where the last one left off.

Contents:
- **Goal**: What are we trying to figure out or create?
- **Current state**: Where did we land? What's been decided?
- **Open questions**: What still needs to be resolved?
- **Key constraints**: Anything that limits options

Update this at the end of each working session, or when significant progress is made.

#### `discussion.md` — The Whiteboard

Running capture of ideas, proposals, and exchanges. Format loosely:

```markdown
## <Date or Topic>

<Ideas, questions, responses, sketches>

---
```

This file can be messy — it's for working, not for posterity.

#### `scratch/` — Working Files

Any drafts, code snippets, diagrams, or experiments. Structure however makes sense for the topic. These are candidates for graduation once they're solid.

#### Resuming a Session

When returning to an existing Storm Session:

1. Read `context.md` first — this has the current state
2. Skim `discussion.md` for recent context if needed
3. Check `scratch/` for any in-progress artifacts
4. Ask clarifying questions if the state is unclear

The human may have done thinking outside the session. Ask what's changed before diving in.

#### Graduating Work

When an idea is fleshed out enough:

1. Move artifacts from `scratch/` to their permanent home (`artifacts/`, `research/`, `docs/`, etc.)
2. Delete the session directory — or leave it temporarily if you might return
3. No need to preserve `discussion.md` — the artifacts carry the value

The human decides when something graduates. If uncertain, ask.

---

### Pre-Mortem

**Invocation**: "Pre-Mortem" or "Let's do a pre-mortem"

**Trigger**: User-initiated, or agent-initiated at significant decision points.

Pre-Mortem is an adversarial review before committing to an approach or decision. The premise: assume the decision has already failed. Why did it fail?

#### When to Trigger (Agent-Initiated)

The agent should proactively suggest a Pre-Mortem when:
- Making architecture or design decisions
- Choosing between tools, frameworks, or approaches
- About to make irreversible changes
- Committing to a direction that will be costly to reverse

Phrase it as: "Before we commit to this, want to do a quick Pre-Mortem?"

#### How to Run a Pre-Mortem

1. **State the decision**: What are we about to commit to?
2. **Assume failure**: Imagine we're 6 months out and this failed. What went wrong?
3. **List failure modes**: Be specific — technical debt, scaling issues, maintenance burden, misunderstood requirements, etc.
4. **Surface alternatives**: What options haven't been fully considered?
5. **Challenge assumptions**: What are we taking for granted that might not hold?
6. **Decision**: Proceed, pivot, or investigate further.

#### Output

The Pre-Mortem doesn't need to be documented unless it surfaces a significant pivot. If it does, capture the decision in `decisions.md` (for Storm Sessions) or as a code comment / commit message (for established work).

---

### Smooth Brain

**Invocation**: "Smooth Brain" or "Let's go smooth brain"

**Trigger**: User-initiated, or agent-initiated at the beginning of any session.

Smooth Brain is a deliberate step back to surface and question assumptions. The name reflects the goal: temporarily forget what you "know" and examine it fresh.

#### Auto-Trigger at Session Start

At the beginning of any working session, the agent should do a quick Smooth Brain check:
- What assumptions are we carrying from previous sessions?
- What context might have changed since last time?
- Are there any "obvious" things that should be questioned?

This can be brief — a few sentences acknowledging the current mental model and asking if it still holds.

#### How to Run Smooth Brain

1. **List current assumptions**: What do we believe to be true about this problem/project/approach?
2. **Source each assumption**: Where did this belief come from? Is it still valid?
3. **Flag stale assumptions**: What might have changed? What should be re-verified?
4. **Update or confirm**: Either update the understanding or explicitly confirm the assumption still holds.
5. **Proceed**: Continue with a clean mental slate.

#### When to Call It Manually

- When stuck or going in circles
- When returning to work after a break
- When something feels "off" but it's unclear what
- When onboarding a new agent to existing work

---

### Progressive Bookkeeping

**Trigger**: Always-on. This is not invoked — it's a constant behavior.

Progressive Bookkeeping means keeping state files current throughout a session, not batching updates for the end. The goal: work can be picked up later or ended abruptly without losing context.

#### What to Keep Current

**During Storm Sessions:**
- `context.md` — update when state changes significantly
- `discussion.md` — update as ideas develop
- `scratch/` files — update as drafts evolve
- `BACKLOG.md` — update if tasks are identified or completed
- `planning/ACTIVITY.md` — update if structural/meta changes occur

**During Established Work:**
- `BACKLOG.md` — update as work progresses
- `planning/ACTIVITY.md` — update if relevant to workspace structure
- Relevant documentation files — update as understanding evolves

#### Update Frequency

Update files **as progress happens**, not at the end. The human may be watching from Neovim or another editor, or the session may end unexpectedly.

Rule of thumb: if you've made meaningful progress, the relevant state files should already reflect it.

#### Why This Matters

- Sessions can end abruptly (crashes, context limits, human walks away)
- Another agent may pick up the work
- The human should be able to see progress in real-time
- Reduces cognitive load of "catching up" on resumed sessions

---

### Dead Drop

**Invocation**: "Dead Drop" or "Bread Crumb"

**Trigger**: User-initiated, or agent-initiated when session is ending.

Dead Drop is an explicit, structured capture of session state for handoffs. It leaves enough context that a future session (same human, different agent, or both) can pick up without re-litigating settled decisions.

#### When to Use

- Mid-session when you need to pause but will return
- End of session as a handoff to future work
- Before context gets too long and risks being lost
- When switching between agents or tools

#### Where It Lives

Dead Drops go in `planning/whats-next.md`.

For Storm Sessions, also update the session's `context.md` — but the Dead Drop in `whats-next.md` is the canonical "resume here" marker.

#### Full Dead Drop (preferred)

```markdown
# Dead Drop — YYYY-MM-DD

**In progress:** [one sentence — what's mid-flight right now]

**Just completed:**
- [bullet]
- [bullet]

**Next step:** [one sentence — what would happen next if the session continued]

**Key decision:** [one sentence — anything that would be re-litigated without this — or "none"]

**Git state:** [short hash] — [last commit message] (or "uncommitted changes" / "clean")

**Open threads:** [any dangling questions or blocked items — or "none"]
```

#### Quick Bread Crumb (fallback)

When there's no time for the full format:

```markdown
> YYYY-MM-DD HH:MM — [what's happening / what's next]
```

Quick crumbs append to the file. A future session can read the trail and reconstruct. **This is a fallback, not default** — use full format when possible.

#### Agent-Initiated

The agent should prompt for a Dead Drop when:
- Session is clearly ending (user says goodbye, wrapping up)
- Context is getting long and state should be preserved
- Switching to a different workstream

Phrase it as: "Want me to drop a bread crumb before we wrap?"

#### Recovery

If a session ends without a Dead Drop, the git log is the fallback — shows what landed, not what was in flight. Clean working tree + recent commits = recoverable state, just less context.

#### Close-out (compound behavior)

**Trigger:** "close-out", "write a handoff", "I'm closing this session"

Close-out is a compound behavior for session endings. It combines Smooth Brain and Dead Drop with a specific focus on *continuation*.

**Flow:**
1. **Smooth Brain (scoped):** What's in this context that won't survive? What's assumed captured but isn't? What would a fresh session need to ask again?
2. **Dead Drop (for continuation):** Write a handoff oriented toward picking up, not just summarizing what happened.

Use Close-out when the session is ending but the work isn't finished. A fresh session should be able to continue without re-litigating settled decisions.

---

### Troubleshoot

**Invocation**: "Troubleshoot" or "Debug"

**Trigger**: User-initiated, or agent-initiated when problem-solving context is detected (error messages, "this isn't working," "I'm having an issue with...").

Troubleshoot is a structured mode for investigating issues and capturing remediation knowledge. Unlike Storm Session (disposable brainstorming), Troubleshoot builds reference material that persists.

#### What Troubleshoot Is For

- Investigating errors, failures, misconfigurations
- Working through problems with back-and-forth debugging
- Capturing what was tried and what worked
- Building a knowledge base of fixes and setup guidance

#### What Troubleshoot Is NOT For

- Brainstorming new features or designs (use Storm Session)
- One-off questions with quick answers (just answer directly)
- Documenting known procedures (use Quick Doc path)

#### Session Structure

Troubleshooting sessions live in `wip/troubleshoot-<system>-<issue>/` during active investigation.

```
wip/troubleshoot-openshift-storage-pvc-pending/
  investigation.md    # Running log — the working document
```

Lightweight by design. One file tracks the whole investigation.

**Naming**: `troubleshoot-<system>-<brief-issue>` — e.g., `troubleshoot-openshift-storage-pvc-pending`, `troubleshoot-docker-network-timeout`.

#### investigation.md — The Running Log

This file is updated progressively as the investigation proceeds (Progressive Bookkeeping applies).

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

#### Graduation — Distributable Documentation

When the investigation resolves, the agent creates distributable documentation.

**Trigger**: "Create a walkthrough" / "Write this up" / agent prompts: "Want me to write this up?"

**Two output types:**

**1. Remediation Doc** (always created)

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

**Graduation decision logic:**

| Agent assessment | Action |
|------------------|--------|
| Prevention doc clearly needed | Create both docs |
| Unsure or probably not needed | Prompt user: "Should I also write a prevention/setup guide?" |
| User says no | Skip prevention doc |

The agent never unilaterally decides to skip the prevention doc. If in doubt, ask.

**Where distributables live:**

```
docs/
  troubleshooting/
    <system>/
      <issue>.md           # Remediation docs
  guides/
    <system>/
      <component>.md       # Prevention / setup docs
```

#### Lifecycle

**Full Troubleshooting Session:**

1. **Detect/Invoke**: Recognize troubleshooting context or explicit trigger
2. **Start session**: Create `wip/troubleshoot-<system>-<issue>/investigation.md`
3. **Investigate**: Update investigation.md progressively
4. **Resolve**: Document root cause and fix in investigation.md
5. **Graduate**: Create remediation doc (always), prevention doc (if needed or user requests)
6. **Discard session**: Delete `wip/` session directory — investigation.md is disposable once docs exist

**Quick Doc Path:**

Not everything needs a full session. Use the quick path when:
- The fix is already known (no investigation needed)
- User asks to document something common ("write up how to configure X")
- Quick question-and-answer that's worth capturing

Quick path: User asks for doc → Agent writes directly to `docs/troubleshooting/` or `docs/guides/` → No `wip/` session needed.

#### Agent Behavior

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
- Assess whether prevention doc is needed — if clearly yes, create it; if unsure, ask user; never skip without user confirmation

---

### Sneaky

**Invocation**: "Sneaky", "Let's be sneaky", "Hide your actions", "Go dark"

**Trigger**: User-initiated. Used when applying this working style in external repositories where visible working directories might be unwanted.

Sneaky mode consolidates the working directories (`wip/`, `planning/`) into a single hidden `.workspace/` directory. This keeps the working style functional while minimizing visible footprint in the repository.

#### When to Use

- Working in a repository that isn't your own
- Contributing to open source where working files would be noise
- Any context where visible `wip/` and `planning/` directories are inappropriate

#### Structure

When Sneaky is invoked, the working directories move under `.workspace/`:

```
.workspace/
  wip/
    <topic-name>/
      context.md
      discussion.md
      scratch/
  planning/
    whats-next.md
    ACTIVITY.md
```

#### Activation

When the user invokes Sneaky:

1. Create `.workspace/` if it doesn't exist
2. Move `wip/` → `.workspace/wip/` (or create if doesn't exist)
3. Move `planning/` → `.workspace/planning/` (or create if doesn't exist)
4. All subsequent working style behaviors use `.workspace/` as the root

#### Deactivation

**Invocation**: "Stop being sneaky", "Come out of hiding", "Go visible"

1. Move `.workspace/wip/` → `wip/`
2. Move `.workspace/planning/` → `planning/`
3. Delete `.workspace/` if empty
4. Resume normal visible mode

#### Agent Behavior

- When Sneaky is active, all file paths in behaviors adjust automatically (e.g., Dead Drops go to `.workspace/planning/whats-next.md`)
- The agent should acknowledge when entering/exiting Sneaky mode
- If `.workspace/` already exists when entering a repo, assume Sneaky mode is active

---

## Conventions

Conventions are always-on rules that shape how work happens. Unlike behaviors, they're not invoked — they apply automatically.

### Isolation

Artifacts created during a session stay in the current project. No automatic write-back to external workspaces.

This workspace may reference or generate content for other repositories, but changes to external projects require explicit confirmation. When in doubt, ask before writing outside the current workspace.

**Exception:** Intentional cross-repo work (e.g., "generate a README for repo X") is permitted when explicitly requested.

### Append, Don't Replace

State files accumulate entries rather than being overwritten. The most recent entry is the active state, but history is preserved.

**Applies to:**
- `planning/whats-next.md` — Dead Drops append, don't replace
- `discussion.md` in Storm Sessions — running log, not overwritten
- Any file serving as a session log or state tracker

**Why:** Provides audit trail and context. A fresh session can scan previous entries to understand trajectory, not just current state.

### Documentation Structure

Graduated documentation lives in `docs/`. Use subdirectories to organize by project or domain.

```
docs/
  troubleshooting/       # Remediation docs (from Troubleshoot sessions)
    <system>/
      <issue>.md
  guides/                # Setup/prevention docs (from Troubleshoot sessions)
    <system>/
      <component>.md
  <project>/             # Project-specific documentation
    README.md            # Overview and entry point
    architecture.md      # Diagrams, layouts, system design
    decisions.md         # Key decisions and rationale
    ...
```

**Naming:** Use kebab-case for directories and files (`homelab/`, `network-topology.md`).

**Entry point:** Each project directory should have a `README.md` that orients the reader — what is this, why does it exist, where to look next.

---

## Working on Established Files

For files that already exist outside `wip/`:

- Edit them directly — no special workflow needed
- Use standard commit practices
- Keep `BACKLOG.md` and `planning/ACTIVITY.md` current (Progressive Bookkeeping)
- If a significant rework is needed, consider whether it's really a new idea (Storm Session) or just iteration on existing work (direct edit)
