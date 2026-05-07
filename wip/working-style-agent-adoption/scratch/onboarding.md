# Working Style Onboarding

This document instructs AI agents to adopt the working style defined in this workspace by creating rules in their native format.

**Do not follow this as a behavior guide.** Instead, use it to generate persistent rules that will guide your future behavior.

---

## Instructions

1. **Read the source**: Load `workstyle/working_style.md` in full. This is the canonical definition of behaviors and conventions.

2. **Identify your rule system**: Determine what rule/instruction format you can create and persist:
   - **Cursor**: `.cursor/rules/*.mdc` files
   - **OpenCode**: `.opencode/rules/*.md` or similar
   - **Gemini**: `.gemini/` configuration
   - **Claude Projects**: Project instructions
   - **Other**: Whatever persistent instruction format is available

3. **Create rules for each behavior**: Generate a rule file for each behavior you'll use. The rule should be written in your own words — how *you* understand and would execute the behavior. Don't copy-paste; translate into instructions that work for your architecture.

4. **Prioritize Progressive Bookkeeping**: This is the most commonly skipped behavior. Your rule for it should include:
   - Specific trigger points (after making progress, before responding, etc.)
   - Which files to update and when
   - A self-check mechanism ("Have I updated state files?")

5. **Create a skill for rule creation**: If your system supports skills/tools, create one that lets you generate new rules from the working style on demand.

---

## Rule Generation Guidelines

When creating rules, optimize for *your* understanding:

- **Be specific**: Vague instructions get ignored. Include concrete file paths, trigger conditions, and outputs.
- **Be concise**: Long rules get skimmed. Capture the essence, not the full explanation.
- **Include examples**: Show what the behavior looks like in practice.
- **Add self-prompts**: Questions you should ask yourself at key moments.

### Example: Progressive Bookkeeping (Cursor format)

```markdown
# Progressive Bookkeeping

**Trigger**: Always-on. After any meaningful progress.

## What to Update

During Storm Sessions:
- `wip/<session>/context.md` — when state changes
- `wip/<session>/discussion.md` — as ideas develop

During any work:
- `planning/whats-next.md` — if priorities shift
- Relevant docs — as understanding evolves

## Self-Check

Before ending a response where you made progress, ask:
> "If this session ended now, would a fresh agent know the current state?"

If no, update the relevant state files before responding.
```

Adapt this to your format. The goal is a rule that actually changes your behavior, not one that documents the behavior.

---

## Behaviors to Create Rules For

| Behavior | Priority | Notes |
|----------|----------|-------|
| Progressive Bookkeeping | **High** | Always-on; most skipped |
| Dead Drop | High | Session handoffs; agent-initiable |
| Storm Session | Medium | Only when invoked |
| Smooth Brain | Medium | Auto-trigger at session start |
| Pre-Mortem | Medium | Agent-initiable at decisions |
| Troubleshoot | Low | Only when debugging |
| Sneaky | Low | Only for external repos |

Start with high-priority behaviors. Add others as needed.

---

## After Onboarding

Once you've created rules:

1. **Verify they load**: Confirm your rules are active in future sessions
2. **Test Progressive Bookkeeping**: Do a small task and check if you update state files without prompting
3. **Iterate**: If a rule isn't working, rewrite it — you understand your own failure modes

The goal is internalized behavior, not just documented behavior.
