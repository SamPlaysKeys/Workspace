## 2026-06-01: Incorporating Templates (Start, Consolidate, Cross-link)

**Action**: Drafted 3 new skills based on user feedback and the `my-ai-workspace` reference. The user prefers keeping massive projects in `wip/` rather than graduating them to `.planning/` via a "brief", so we adapted the concept into a `consolidate` skill.

**New Skill Drafts**:
1. **`start-skill.md`**: Provides a strict, templated orientation at the beginning of a session ("Where Things Stand", "Handoff", "Suggested Focus"). Puts the user in control of what happens next.
2. **`cross-link-skill.md`**: An anti-orphan convention. Forces the agent to link any newly created document into the appropriate `README.md` or index.
3. **`consolidate-skill.md`**: Replaces the reference's `brief` skill. Instead of moving work, it tames messy `wip/<topic>/` folders by generating a `BRIEF.md` *inside* the WIP folder. This provides an Abstract, Scope limits, and a Table of Contents for the scattered scratch files.

**Current Roster in `scratch/`**:
- `ideate-skill.md`
- `troubleshoot-skill.md`
- `document-skill.md`
- `handoff-skill.md`
- `start-skill.md`
- `cross-link-skill.md`
- `consolidate-skill.md`

**Next Steps**: 
Review this roster. Does `consolidate` hit the right note for managing large WIP folders? Are there any remaining pieces of the "running off" puzzle to solve?