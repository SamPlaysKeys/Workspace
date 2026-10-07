# Context: Skill Creation & Updates

**Goal**: Review existing skills from a previous project, assess their value/use for incorporation into this workspace as reusable artifacts and active skills, and establish workflows/patterns for creating and updating skills.
**Current State**: Drafted initial specification for `scrub` skill in `wip/skill-creation-and-updates/scrub-skill-draft.md` covering secrets, infra, and ident targets with an interactive remediation workflow.
**Open Questions**:
- Does the draft specification for `scrub` meet the user's requirements?
- Are there any specific placeholder conventions or tooling integrations (e.g. gitleaks / git diff mode) to include?
- Once approved, should we graduate `scrub` to `.agents/skills/scrub/` and sync `workstyle/working_style.md` and `AGENTS.md`?
**Key Constraints**:
- Work is quarantined in `wip/skill-creation-and-updates/` during ideation.
- Core workspace conventions and portability requirements apply (e.g., any updates to core behaviors must remain in sync across `.agents/skills/`, `AGENTS.md`, and `workstyle/working_style.md`).
