# Working Style: Co-Creation Conventions

This document defines how AI agents should collaborate in this workspace. These conventions are agent and model agnostic — any AI assistant working here should follow them.

---

## Storm Sessions (`.wip/`)

The `.wip/` directory is for **Storm Sessions** — focused brainstorming on net-new ideas. Think of it as a whiteboard for rapid ideation before committing to deeper work.

### What Storm Sessions Are For

- Brainstorming and exploring new concepts
- Drafting abstracts, outlines, and initial structures
- Quickly fleshing out ideas before deeper research
- Collaborative ideation that benefits from back-and-forth

### What Storm Sessions Are NOT For

- Editing or reworking established files (just edit those in place)
- Long-term project management
- Permanent documentation

### Session Lifecycle

1. **Start**: Create a topic-based session in `.wip/`
2. **Iterate**: Workshop the idea across one or more conversations
3. **Graduate**: Move valuable artifacts to their permanent homes
4. **Discard**: Delete the session — the brainstorming itself doesn't need to persist

Sessions are **disposable by design**. The output matters; the messy process doesn't need to be in version control.

---

## Session Structure

```
.wip/
  <topic-name>/
    context.md       # What we're exploring, current state, open questions
    discussion.md    # Running log of ideas and exchanges
    scratch/         # Draft files, snippets, experiments
```

**Naming**: Use the topic or concept name (`api-gateway-design`, `k8s-monitoring-strategy`, `blog-post-ci-cd`).

### `context.md` — The Resumption File

This is the most important file for continuity. It should allow any agent to pick up where the last one left off.

Contents:
- **Goal**: What are we trying to figure out or create?
- **Current state**: Where did we land? What's been decided?
- **Open questions**: What still needs to be resolved?
- **Key constraints**: Anything that limits options

Update this at the end of each working session, or when significant progress is made.

### `discussion.md` — The Whiteboard

Running capture of ideas, proposals, and exchanges. Format loosely:

```markdown
## <Date or Topic>

<Ideas, questions, responses, sketches>

---
```

This file can be messy — it's for working, not for posterity.

### `scratch/` — Working Files

Any drafts, code snippets, diagrams, or experiments. Structure however makes sense for the topic. These are candidates for graduation once they're solid.

---

## Resuming a Session

When returning to an existing Storm Session:

1. Read `context.md` first — this has the current state
2. Skim `discussion.md` for recent context if needed
3. Check `scratch/` for any in-progress artifacts
4. Ask clarifying questions if the state is unclear

The human may have done thinking outside the session. Ask what's changed before diving in.

---

## Graduating Work

When an idea is fleshed out enough:

1. Move artifacts from `scratch/` to their permanent home (`artifacts/`, `research/`, `docs/`, etc.)
2. Delete the session directory — or leave it temporarily if you might return
3. No need to preserve `discussion.md` — the artifacts carry the value

The human decides when something graduates. If uncertain, ask.

---

## Update Frequency

During active Storm Sessions, **update files frequently**. The human may be watching from Neovim or another editor.

- Update `discussion.md` as ideas develop
- Update `context.md` when state changes significantly
- Update `scratch/` files as drafts evolve

---

## Working on Established Files

For files that already exist outside `.wip/`:

- Edit them directly — no special workflow needed
- Use standard commit practices
- If a significant rework is needed, consider whether it's really a new idea (Storm Session) or just iteration on existing work (direct edit)
