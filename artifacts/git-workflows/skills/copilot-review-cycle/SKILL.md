---
name: copilot-review-cycle
description: >
  Review Copilot PR feedback comments, assess each one (valid / false positive /
  partially valid), act on valid ones (fix + commit), and reply to every comment
  with what was done and why. Use when: a Copilot review has been posted on a PR
  and the user asks to address, review, or action the feedback.
  Keywords: copilot review, address feedback, review comments, fix or dismiss,
  respond to review.
license: Apache-2.0
compatibility: >
  Requires gh CLI auth and git access to the PR branch worktree.
metadata:
  author: platform-team
  version: "1.0"
allowed-tools: Bash Read Edit
---

# Copilot Review Cycle

Work through every Copilot inline review comment on a PR: assess, act, and
reply. No comment is left unanswered.

---

## Step 1 — Fetch all inline review comments

```bash
gh api repos/<owner>/<repo>/pulls/<pr>/comments \
  | python3 -c "
import sys, json
for c in json.load(sys.stdin):
    print(f'ID:{c[\"id\"]}  {c[\"path\"]}:{c.get(\"line\", c.get(\"original_line\",\"?\"))}')
    print(c['body'])
    print()
"
```

Collect all comment IDs — you must reply to **every** one by the end.

---

## Step 2 — Read the code at each flagged location

Use `view` with `view_range` on the exact file and line. Do not rely on the
comment text alone — read the real code before forming a verdict.

---

## Step 3 — Assess each comment

For each comment, assign one of three verdicts before acting:

| Verdict | Meaning | Action |
|---|---|---|
| **Valid** | Bug, correctness issue, or clear improvement | Fix the code |
| **Partially valid** | Concern is real but the suggested fix is wrong or overkill | Fix in a better way and explain |
| **False positive** | Code is already correct; comment misread the logic | No code change; reply with explanation |

Assessment notes to always consider:
- **YAML block scalars** (`|`, `|-`, `>-`): indentation stripping rules affect
  how Jinja2 expressions and `\n` join separators actually render. Trace
  carefully before accepting an alignment complaint.
- **Jinja2 `default()`**: `default(x)` fires only on undefined; `default(x, true)`
  also catches empty string. Check which behaviour the code actually needs.
- **Defensive vs. active bug**: distinguish "this could fail with a malformed
  input that doesn't exist in any real inventory" from "this will fail in
  production". Both may warrant a fix, but the framing matters.

---

## Step 4 — Act on valid / partially valid comments

Make the minimal correct fix. Then:

```bash
cd <worktree>
ansible-lint <changed path>   # for Ansible files
git add <file>
git commit -m "fix: <what and why>\n\nCo-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>"
git push
```

---

## Step 5 — Reply to every comment (mandatory)

After acting (or deciding not to act), post a reply to **each** comment ID.
Never leave a comment without a response.

```bash
gh api repos/<owner>/<repo>/pulls/comments/<comment_id>/replies \
  -X POST \
  -f body="<reply text>"
```

**Reply templates:**

_Fixed:_
> Fixed in `<sha>` — `<one line description of what changed and why>`.

_False positive:_
> Not a bug — `<concise explanation of why the code is correct as written>`.

_Partially valid / fixed differently:_
> Valid concern, but fixed differently: `<explanation>`. Applied in `<sha>`.

Keep replies concise and technical. Avoid "Thanks for the feedback" filler.

---

## Step 6 — Verify

```bash
gh api repos/<owner>/<repo>/pulls/<pr>/comments \
  | python3 -c "
import sys, json
for c in json.load(sys.stdin):
    print(c['id'], '—', c['path'], '—', c.get('line','?'))
"
# Confirm every comment ID has a reply in the thread
```

Confirm no comment IDs are missing a reply before closing out.
