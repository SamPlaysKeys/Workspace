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

Troubleshooting sessions live in `wip/troubleshoot-<system>-<issue>/` during active investigation.

## Workflow

1. **Trigger:** "troubleshoot this", "debug this", or pasting an error log
2. **Start session:** Create `wip/troubleshoot-<system>-<issue>/investigation.md`
3. **Investigate:**
   - Log symptoms
   - Form hypothesis
   - Test & record results
   - Repeat until resolved
4. **Resolve:** Document the root cause and the exact fix
5. **Graduate:** 
   - Create `docs/troubleshooting/<system>/<issue>.md` (using standard template)
   - (Optional) Create `docs/guides/<system>/<prevention>.md` if it's a setup/prevention issue
6. **Discard session:** Delete `wip/` directory — investigation.md is disposable once docs exist

## Fast Path (Quick Doc)
If the issue is known or solved immediately without needing a scratchpad:
1. Skip `wip/` session
2. Write directly to `docs/troubleshooting/`
3. No `wip/` session needed

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
