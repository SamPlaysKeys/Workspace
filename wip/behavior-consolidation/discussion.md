## 2026-06-01: Handoff / Breadcrumb Skill

**Action**: Converted the "Dead Drop" behavior into a `handoff` skill.

**Changes**:
- **`handoff-skill.md`**: Created to handle session state capture.
- **Triggers**: Explicitly mapped "drop a breadcrumb", "leave a breadcrumb", "dead drop", and "close out" to this skill so the agent knows exactly when to invoke it based on natural language.
- **Templates**: Brought over the "Full Dead Drop" and "Quick Bread Crumb" templates from the original working style, instructing the agent to append them to `planning/whats-next.md`.

**Next Steps**: 
- Review the remaining behaviors: Storm Session, Pre-Mortem, Sneaky.