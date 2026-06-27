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
---

# Discussion — Drift Management System

## Refined problem statement
Manual changes happen in Argo-managed clusters. The system needs to convert those manual edits into reviewable pull requests and then restore cluster state before drift persists.

## Required behavior
- PR is the recovery mechanism
- rollback is the anti-drift enforcement
- schedule is the orchestrator

## Rhocm-vs-Argo decision surface
### Rhocm
- likely stronger for multi-cluster observability and policy-driven actions
- may require adapting remediation primitives to PR creation
- best fit if the user wants enterprise-scale governance
### Argo
- closest to the origin of the drift
- diff and sync semantics already model manifests versus live state
- likely simpler if the workflow is one repo per app and one cluster target

## PR creation requirements
- branch naming convention tied to app or cluster
- commit message template with context
- linked evidence; diff or change summary from live state
- idempotent reopen behavior if rollback time expires without merge

## Rollback requirements
- reversible snapshot or commit reference
- webhook or status check before destructive rollback
- preflight validation to confirm PR is still open
- escalation path if rollback fails

## schedule-plus-grace-term model
- phase 1: detection and PR opening
- phase 2: reviewer grace term
- phase 3: rollback if not merged
- configurable durations per app and per risk class

## Risk surface
- manual changes may include security fixes; blind rollback is unsafe
- PR flooding from noisy resources
- race between reviewer merge and rollback job
- permission boundaries between git write and cluster apply
