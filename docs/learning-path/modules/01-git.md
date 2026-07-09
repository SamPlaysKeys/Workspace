# Module 1 — Git, GitHub & GitLab Best Practices

**Audience:** Anyone using Git day to day — developers, writers, infrastructure and platform engineers. If you've used Git but not internalized *why* the model works, this module is for you.

**Outcomes:** Understand how Git stores history (the inside-out mental model); use the daily workflow (clone → branch → commit → push → PR/MR → merge) without looking up commands; reason about `git log`, `diff`, `revert`; operate comfortably in both GitHub and GitLab with branch protection, CODEOWNERS, and SSO/SAML; and earn the **GitHub Foundations** certification.

---

## Checklist — work through in order

- [ ] **Stage 0 (do first):** watch [Git For Ages 4 And Up](https://www.youtube.com/watch?v=1ffBJ4sVUb4) and draw the object model
- [ ] **Stage 1:** Microsoft Learn — GitHub Fundamentals path + [GitHub Skills](https://skills.github.com/) (in-browser); GitLab users add [GitLab for Beginners](https://about.gitlab.com/learn/gitlab-for-beginners/)
- [ ] **Stage 2:** finish [learngitbranching.js.org](https://learngitbranching.js.org/) "Introduction Sequence" + "Ramping Up"; read [Pro Git](https://git-scm.com/book/en/v2) Ch.1–3
- [ ] Work the **Mechanics checklist** (clone/branch/commit/push/PR, diff, log, revert vs reset, CODEOWNERS, branch protection)
- [ ] Complete **enterprise hosting** onboarding (SSO-authorized PAT, 2FA) <!-- ORG-SPECIFIC: SSO/SAML, token policy, approved CLIs -->
- [ ] **Certify:** book **GitHub Foundations** (GitHub/PSI · $99 · 75 questions · 120 min · 700/1000 · no prerequisites). Study aids: Microsoft Learn GitHub Fundamentals, [Pro Git](https://git-scm.com/book/en/v2), [LadyKerr github-certification-guide](https://github.com/LadyKerr/github-certification-guide) <!-- ORG-SPECIFIC: exam voucher/provider -->
- [ ] Pass the scenario-based Verification at the bottom

---

## Mental model first — not just "how to Git"

Most Git tutorials start with commands. This module starts with the *model*, because commands make no sense without it. Git stores **snapshots**, not diffs; commits are immutable, content-addressed objects; branches are **labels**, not containers. Every confusing moment (detached HEAD, merge vs rebase, rejected pushes) is a model mismatch.

**Do Stage 0 before anything else.**

### Git vs Git hosting platforms

- **Git** = decentralized version control (created by Linus Torvalds, 2005). Every developer has the full history. Identical everywhere it's installed.
- **GitHub / GitLab** = hosting services *on top of* Git: web UI, PRs/MRs, issues, CI/CD, wikis. The collaborative layer differs; Git itself does not care which host you use.
- **Core workflow is identical on both:** `clone` → `checkout -b feature` → `add` → `commit` → `push -u origin feature` → open PR (GitHub) / MR (GitLab).

| GitHub | GitLab | Meaning |
|--------|--------|---------|
| Pull Request (PR) | Merge Request (MR) | Proposed, reviewed change |
| GitHub Actions | GitLab CI | CI/CD (GitLab built-in, GitHub opt-in) |
| Organizations | Groups | Team management |
| `gh` CLI | `glab` CLI | Terminal tooling |

### Git as the tool for change management

Git doesn't create governance — the workflows on top do. This reframe is the cultural core of everything later (especially GitOps).

| Traditional process | Git / hosting-platform equivalent |
|---------------------|-----------------------------------|
| Change ticket | **PR / MR** — the diff *is* the change; description is the body |
| Peer review / approval | **PR/MR review** — named, required approvers via branch protection |
| Approval record | **Merge commit** — named, timestamped, traceable |
| "Who changed this?" | `git log`, `git blame`, PR history |
| Emergency / break-glass | Emergency PR with post-hoc review; audit trail preserved |
| Rollback | `git revert` — both change and rollback preserved in history |

Git makes governance faster, more traceable, and automatable at the enforcement layer.

---

## Stage 0 — Mental model (free, ~2h)

**[Git For Ages 4 And Up — Michael Schwern](https://www.youtube.com/watch?v=1ffBJ4sVUb4)** (~1h 40m). Teaches objects → commits → labels → staging → remotes with physical props. After this, detached HEAD, rebase, and `reset` become logical consequences of the model.

**Verification:** On paper/whiteboard, draw the relationship between a blob, a tree, a commit object, a branch label, HEAD, and a remote ref. Explain what `git commit` physically does to those objects.

---

## Stage 1 — Hands-on basics (free, 2–4h)

| Resource | What it covers | Link |
|----------|---------------|------|
| **Microsoft Learn — GitHub Fundamentals** | Structured, exam-aligned path: Git basics, GitHub flow, collaboration, PRs, issues, project boards | <!-- ORG-SPECIFIC: link our assigned Microsoft Learn GitHub Fundamentals path --> [GitHub Fundamentals (Microsoft Learn)](https://learn.microsoft.com/) |
| **GitHub Skills** (in-browser) | Introduction to Git; Introduction to GitHub — automated feedback via Actions | [skills.github.com](https://skills.github.com/) |
| **GitLab for Beginners** (in-browser) | Same fundamentals in the GitLab UI | [about.gitlab.com/learn/gitlab-for-beginners](https://about.gitlab.com/learn/gitlab-for-beginners/) |
| **Microsoft Learn — Introduction to Git** | Modules: Intro to Git (~1h26m), Collaborate with Git (44m), Introduction to GitHub | [learn.microsoft.com/.../intro-to-git](https://learn.microsoft.com/en-us/training/modules/intro-to-git/) |

**Verification:** Clone a repo, create a branch, edit one line, commit, push, open a PR/MR. Write the description as a change ticket: what changed, why, risk, how to verify.

---

## Stage 2 — Internals & interactive practice (free)

| Resource | Covers | Notes |
|----------|--------|-------|
| **[learngitbranching.js.org](https://learngitbranching.js.org/)** | Branching, merging, rebasing, cherry-pick — visually | Start with "Introduction Sequence" |
| **[Pro Git book](https://git-scm.com/book/en/v2)** (free) | Everything; Ch.1–3 fundamentals, Ch.10 internals | Deeper chapters for submodules, rerere, merge strategies |

**Verification:** Complete "Introduction Sequence" + "Ramping Up" of learngitbranching. Explain, without notes, `git rebase main` vs `git merge main` on a feature branch.

---

## Mechanics checklist (Day-to-day GitOps work)

| Topic | Why it matters |
|-------|---------------|
| Install Git; set `user.name` / `user.email` | Every commit attributed; matches policy/audit |
| **Clone**, **remote**, **fetch** vs **pull** | GitOps tooling reads from remotes; needs reproducible local copy |
| **Branch / commit / push** | Feature branch → PR → merge to `main`/env branch |
| **PR/MR lifecycle** | Where review/approval gate automation |
| **Diff** (`git diff`, IDE) | Constantly compare YAML to cluster behavior |
| `git log` / `git log --oneline --graph` | "What changed and when?" |
| `git stash` | Shelve WIP when switching branches |
| `git revert` vs `git reset` | `revert` safe on shared branches; `reset` rewrites history (local only) |
| Org basics: **permissions, CODEOWNERS, branch protection** | You may not push to `main` — that is correct and intentional |
| Optional: **`gh` / `glab` CLI** | Open PRs/MRs, check CI from terminal |

---

## Enterprise hosting notes

- Complete IT device/token onboarding *before* pushing — PATs often need one-time SSO authorization.
- Prefer **fine-grained PATs** (or deploy keys / service-account tokens for CI) over classic repo-scoped tokens.
- <!-- ORG-SPECIFIC: our SSO/SAML provider, token policy, required 2FA, approved CLI tools. -->

---

## Certification

| Certification | Aligns with | Notes |
|--------------|-------------|-------|
| **GitHub Foundations** | Stage 0–2 + enterprise notes | Validates Git + GitHub fundamentals; pair with the Microsoft GitHub Fundamentals learn path as study material. Exam booked via <!-- ORG-SPECIFIC: our exam voucher / provider --> |

---

## Verification (scenario-based)

1. **Clone → branch → edit → push → PR:** as above, with ticket-style description.
2. **Explain the model:** without notes — `git commit` (local) vs `git push` (publish); `git pull` (laptop) vs merging a PR/MR (review gates, approval record, audit).
3. **Read history:** from `git log`, answer who changed `deployment.yaml` two weeks ago, what, and was it reviewed?
4. **Recover:** make a bad commit on a feature branch; `git revert` it; explain why not `git reset` on a shared branch.


