---
created_at: 2026-06-27
updated_at: 2026-06-27
owner: SamPlaysKeys
topic: drift-management-rhcm-argo
type: Note
status: exploring
review_status: awaiting_review
agent_generated: true
last_modified_by: agent
tags:
  - process
  - planning
---

# Process Plan — Drift Management System

## Assumed toolchain
- ArgoCD source of truth: Git repo with app-of-apps
- Test Argo instance available for rendering desired state
- `argocd-diff-preview` available for comparing two desired-states in GitHub
- Need to compare rendered desired state against live cluster state

## High-Level Process
1. Render manifests for target application(s) from test Argo instance
2. Capture live cluster manifests for same applications
3. Normalize both sides into comparable manifest trees
4. Run diff using `argocd-diff-preview` or equivalent
5. Classify drift
6. Open PR with best-guess changes
7. Schedule grace term before rollback

## Step Detail: Render desired state
- Trigger test Argo instance to render app-of-apps manifests
- Use Argo render API or `argocd app renders` equivalent
- Output should be a set of manifests per application
- This gives us the authoritative intended state from Git

## Step Detail: Capture live state
- Use `kubectl get` for target resources by label/annotation
- Alternative: Argo export or live manifest dump
- Preserve metadata: resourceVersion, annotations, finalizers
- This gives us the actual cluster state

## Wrinkle to resolve
- `argocd-diff-preview` accepts two desired states, not live state directly
- Live state must be converted to a comparable manifest tree
- Options:
  - `kubectl get -o yaml` + prune non-diffable fields
  - Argo diff command if available in your stack
  - Custom lightweight exporter that outputs dry-run-compatible manifests

## Step Detail: Normalize manifests
- Strip non-deterministic fields: resourceVersion, uid, creationTimestamp, managedFields, status
- Align formatting and ordering
- Handle missing fields as “should be deleted”

## Step Detail: Diff and classify
- Run diff tool against normalized manifests
- Classify by:
  - resource kind and path
  - namespace and app boundary
  - known benign patterns (HPA replicas, timestamp annotations)
- Produce drift report with enough context for PR

## Step Detail: PR and grace term
- Under schedule-plus-grace-term model:
  - create branch with detected live state
  - open PR to bring Git repo toward observed state
  - grace term timer starts
- Pre-rollback check: PR still open? Rollback not blocked?

## Step Detail: Rollback
- If grace term expires without merge:
  - revert live state to match rendered desired state
  - use apply with force/recreate if necessary
  - log rollback action with timestamp and drift id

## Validation Plan
1. Manual test with one app: render -> capture -> diff -> classify -> PR -> rollback
2. Measure false positive rate
3. Tune grace term and classifier rules
4. Scale to app-of-apps root if noise is acceptable
