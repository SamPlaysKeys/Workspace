---
type: Reference
---
# Chaining GitHub Actions Workflows Without a PAT or GitHub App

How to trigger one workflow from another when you can't (or don't want to) use Personal Access Tokens or GitHub Apps.

---

## The Problem

You have two workflows:

1. **Workflow A** — Detects branches and auto-creates PRs against main
2. **Workflow B** — Runs tests on PRs opened against main

Manual PRs trigger Workflow B. But when Workflow A creates a PR, Workflow B never runs.

### Why This Happens

GitHub Actions has a deliberate security feature:

> **Actions performed using the default `GITHUB_TOKEN` do not trigger other workflows.**

This prevents accidental infinite loops (Workflow A triggers B triggers A triggers B...). When Workflow A creates a PR using `GITHUB_TOKEN`, GitHub suppresses the `pull_request` event that Workflow B listens for.

---

## Mitigation Options

| Option | How It Works | Pros | Cons |
|--------|--------------|------|------|
| **Personal Access Token** | Use a PAT instead of `GITHUB_TOKEN` | Simple | Tied to user account, expires, requires secret management |
| **GitHub App** | Create app, use its token | Not tied to user, fine-grained permissions | Requires app creation/management, may need org admin access |
| **`workflow_run` trigger** | Workflow B triggers on Workflow A completion | No extra tokens, works within default permissions | Slightly more complex logic |

### Why Not PAT or GitHub App?

In enterprise environments, engineers often:

- Don't have permission to create GitHub Apps at the org level
- Can't or shouldn't create PATs tied to personal accounts for shared workflows
- Face security policies that restrict token creation
- Need solutions that work within the default `GITHUB_TOKEN` permissions

The `workflow_run` approach works entirely within GitHub's default permission model.

---

## Solution: Using `workflow_run` Trigger

Instead of Workflow B listening for `pull_request` events (which won't fire), have it listen for Workflow A's completion.

### Architecture

```
┌─────────────────┐         ┌─────────────────┐
│   Workflow A    │         │   Workflow B    │
│ (Auto-Create PR)│         │  (Run Tests)    │
└────────┬────────┘         └────────▲────────┘
         │                           │
         │ completes                 │ triggers on
         │                           │ workflow_run
         ▼                           │
    ┌────────────┐                   │
    │   PR       │───────────────────┘
    │  Created   │   (B finds PR from A's run)
    └────────────┘
```

### Implementation

#### Workflow A: Auto-Create PR

```yaml
# .github/workflows/auto-pr.yml
name: Auto Create PR

on:
  push:
    branches:
      - 'feature/**'
      - 'fix/**'

jobs:
  create-pr:
    runs-on: ubuntu-latest
    outputs:
      pr-number: ${{ steps.create-pr.outputs.pull-request-number }}
      pr-url: ${{ steps.create-pr.outputs.pull-request-url }}
    steps:
      - uses: actions/checkout@v4

      - name: Create Pull Request
        id: create-pr
        uses: peter-evans/create-pull-request@v5
        with:
          title: "Auto PR: ${{ github.ref_name }}"
          body: |
            Automated PR for branch `${{ github.ref_name }}`
            
            Created by workflow run: ${{ github.run_id }}
          base: main
          branch: ${{ github.ref_name }}

      - name: Output PR info
        run: |
          echo "Created PR #${{ steps.create-pr.outputs.pull-request-number }}"
          echo "URL: ${{ steps.create-pr.outputs.pull-request-url }}"
```

#### Workflow B: Run Tests on Workflow A Completion

```yaml
# .github/workflows/pr-tests.yml
name: PR Tests

on:
  # Still trigger on manual PRs
  pull_request:
    branches: [main]

  # Also trigger when Auto Create PR workflow completes
  workflow_run:
    workflows: ["Auto Create PR"]
    types: [completed]

jobs:
  test:
    runs-on: ubuntu-latest
    # Only run if workflow_run was successful (skip if Workflow A failed)
    if: |
      github.event_name == 'pull_request' ||
      (github.event_name == 'workflow_run' && github.event.workflow_run.conclusion == 'success')
    
    steps:
      - name: Get PR info (for workflow_run trigger)
        id: pr-info
        if: github.event_name == 'workflow_run'
        uses: actions/github-script@v7
        with:
          script: |
            // Find PRs associated with the triggering workflow's head branch
            const headBranch = context.payload.workflow_run.head_branch;
            const headSha = context.payload.workflow_run.head_sha;
            
            const { data: prs } = await github.rest.pulls.list({
              owner: context.repo.owner,
              repo: context.repo.repo,
              head: `${context.repo.owner}:${headBranch}`,
              state: 'open'
            });
            
            if (prs.length === 0) {
              core.setFailed(`No open PR found for branch ${headBranch}`);
              return;
            }
            
            const pr = prs[0];
            core.setOutput('pr-number', pr.number);
            core.setOutput('pr-head-sha', pr.head.sha);
            core.setOutput('pr-head-ref', pr.head.ref);
            
            console.log(`Found PR #${pr.number} for branch ${headBranch}`);

      - name: Checkout PR code
        uses: actions/checkout@v4
        with:
          # Use PR ref for workflow_run, default for pull_request
          ref: ${{ github.event_name == 'workflow_run' && steps.pr-info.outputs.pr-head-ref || github.head_ref }}

      - name: Run tests
        run: |
          echo "Running tests..."
          # Your test commands here
          npm test  # or pytest, go test, etc.

      - name: Report status to PR
        if: github.event_name == 'workflow_run'
        uses: actions/github-script@v7
        with:
          script: |
            const prNumber = '${{ steps.pr-info.outputs.pr-number }}';
            const sha = '${{ steps.pr-info.outputs.pr-head-sha }}';
            
            // Create a check run or comment on the PR
            await github.rest.repos.createCommitStatus({
              owner: context.repo.owner,
              repo: context.repo.repo,
              sha: sha,
              state: 'success',  // or 'failure' based on test results
              context: 'PR Tests (via workflow_run)',
              description: 'Tests passed'
            });
```

### Key Points

1. **Dual triggers**: Workflow B responds to both `pull_request` (manual PRs) and `workflow_run` (auto PRs)

2. **Conditional logic**: The `if` condition ensures we only run on successful Workflow A completions

3. **PR discovery**: Since `workflow_run` doesn't have PR context, we query the API to find the PR by branch name

4. **Status reporting**: We manually create commit statuses so the PR shows test results

---

## Caveats

### Check Runs and PR Status

When triggered via `workflow_run`, the workflow runs in the context of the default branch, not the PR. This means:

- PR checks list won't automatically show the workflow
- You need to manually create commit statuses (shown above)
- Or use the Checks API to create check runs

### Race Conditions

If Workflow A completes before the PR is fully created (rare but possible), the PR lookup might fail. Add a short delay or retry logic if needed:

```yaml
- name: Wait for PR to be indexed
  run: sleep 5
```

### Branch Name Matching

The PR lookup uses the head branch. Ensure Workflow A's branch naming is predictable so Workflow B can find the right PR.

---

## Verification

After implementing:

1. Push a branch matching your pattern (e.g., `feature/test-chain`)
2. Confirm Workflow A runs and creates the PR
3. Confirm Workflow B triggers on Workflow A completion
4. Confirm tests run and status appears on the PR

---

## Summary

The `workflow_run` trigger lets you chain workflows without PATs or GitHub Apps. It's slightly more complex than the token-based approaches, but works entirely within GitHub's default permission model — making it ideal for enterprise environments where token creation is restricted.
