---
type: README-Note
---

# Git Workflows — Agent Skills for PR Lifecycle & Review Triage

A collection of portable Agent Skills designed to streamline pull request hygiene, automated code review triage, and branch synchronization when collaborating with AI agents.

---

## Directory Structure

```text
artifacts/git-workflows/
├── README.md                 # This guide
└── skills/
    ├── copilot-review-cycle/ # Review, fix, and reply to Copilot/automated PR comments
    └── pr-drift-sync/        # Synchronize PR titles and descriptions with branch diffs
```

---

## Skills Overview

### 1. `copilot-review-cycle`
- **Objective:** Systematically process every automated or Copilot inline review comment on a pull request.
- **Workflow:**
  1. Fetches all inline PR review comments via `gh api`.
  2. Reads the relevant lines in the worktree.
  3. Classifies each feedback comment into one of three verdicts:
     - **Valid**: Bug or code improvement &rarr; fix code and commit.
     - **Partially valid**: Real concern, but suggested fix is inaccurate &rarr; apply proper fix and document reasoning.
     - **False positive**: Code is correct &rarr; no edit; explain why in reply.
  4. Replies to every inline thread using `gh api repos/<owner>/<repo>/pulls/<pr>/comments/<id>/replies`.
- **Triggers:** "review copilot comments", "address pr feedback", "respond to copilot review".

### 2. `pr-drift-sync`
- **Objective:** Prevent pull request descriptions and titles from going stale after long iterative development sessions.
- **Workflow:**
  1. Inspects real branch commit history (`base..head`) and changed files.
  2. Synthesizes commit themes, architectural decisions, and current test verification status.
  3. Uses safe heredoc formatting (`gh pr edit <id> --title ... --body-file -`) to prevent shell quoting bugs and markdown truncation.
  4. Confirms updated PR state via `gh pr view`.
- **Triggers:** "sync pr description", "update pr title", "fix pr drift", "refresh pr summary".

---

## Prerequisites

- **GitHub CLI (`gh`)**: Authenticated with repository write permissions (`gh auth status`).
- **Git**: Working tree checked out to the feature branch.
- **Python 3**: For reliable parsing of nested GitHub API JSON outputs.
