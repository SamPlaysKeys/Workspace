# GitHub Actions: Auto-PR Not Triggering Test Workflow

**Status:** resolved

**Environment:** GitHub Actions

---

## Symptoms

- Workflow A: Detects branches with specific names, auto-opens PR against main
- Workflow B: Triggers on PRs opened against main, runs tests
- Manual PR → Workflow B triggers ✓
- Workflow A creates PR → Workflow B does NOT trigger ✗

---

## Investigation

### Hypothesis 1: GITHUB_TOKEN limitation

GitHub Actions has a deliberate security feature: **actions performed using the default `GITHUB_TOKEN` do not trigger other workflows**. This prevents accidental infinite loops.

From GitHub docs:
> "When you use the repository's GITHUB_TOKEN to perform tasks, events triggered by the GITHUB_TOKEN will not create a new workflow run."

**This is almost certainly the issue.**

---

## Resolution Options

### Option A: Use a Personal Access Token (PAT)

Create a PAT with `repo` scope and use it instead of `GITHUB_TOKEN` when creating the PR.

```yaml
- name: Create PR
  uses: peter-evans/create-pull-request@v5
  with:
    token: ${{ secrets.PAT_TOKEN }}  # Not GITHUB_TOKEN
```

**Pros:** Simple, works  
**Cons:** PAT is tied to a user account, expires, needs management

### Option B: Use a GitHub App

Create a GitHub App, install it on the repo, and use its token.

```yaml
- name: Generate token
  id: app-token
  uses: actions/create-github-app-token@v1
  with:
    app-id: ${{ secrets.APP_ID }}
    private-key: ${{ secrets.APP_PRIVATE_KEY }}

- name: Create PR
  uses: peter-evans/create-pull-request@v5
  with:
    token: ${{ steps.app-token.outputs.token }}
```

**Pros:** Not tied to a user, finer-grained permissions, doesn't expire  
**Cons:** More setup (create app, install, manage keys)

### Option C: Use `workflow_run` trigger

Have Workflow B trigger on `workflow_run` completion of Workflow A instead of PR open.

```yaml
# Workflow B
on:
  workflow_run:
    workflows: ["Auto PR Creator"]
    types: [completed]
```

Then query for the PR that was created.

**Pros:** No extra tokens  
**Cons:** More complex logic to find the PR, less intuitive

---

## Recommendation

**Option A (PAT)** is the quickest fix for a personal/homelab context.

**Option B (GitHub App)** is better for org/team contexts or if you want cleaner audit trails.

---

## Verification

After implementing fix:
1. Push a branch matching the naming pattern
2. Confirm Workflow A creates the PR
3. Confirm Workflow B triggers on that PR

---

## Resolution

Chose **Option C (`workflow_run` trigger)** for enterprise compatibility — no PAT or GitHub App required.

Full guide written: `docs/guides/github-actions/chaining-workflows-without-pat.md`

---

## Prevention

Document in repo that auto-PR workflows need either:
- Non-default tokens (PAT or GitHub App), OR
- `workflow_run` trigger pattern for downstream workflows
