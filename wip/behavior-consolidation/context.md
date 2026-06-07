# Context: Behavior Consolidation & Troubleshooting Skills

**Goal**: Pivot from just consolidating existing behaviors to creating a suite of skills that support interactive troubleshooting and discovery. We want to define how to ask questions, how to troubleshoot, and provide templates for the output.

**Current state**: 
- Reviewed inspiration from `~/Src/my-ai-workspace/.agents/skills/`.
- Inspiration skills use a structured format: YAML frontmatter, `<objective>`, `## Context`, `## Process`, and specific markdown templates for output.
- We are shifting focus to build a "Troubleshooting/Discovery Suite" of skills.

**Open questions**: 
- What specific skills make up this suite? (e.g., `diagnose`, `explore-system`, `root-cause`, `remediate`)
- How do we structure the interactive Q&A part of troubleshooting?
- What should the output templates look like for a troubleshooting session vs. a discovery session?

**Key constraints**: 
- Work slow.
- Talk things through.
- Second-guess everything.
- Output must include templates for how the agent should respond.