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

# Context — RHACM / Argo Drift Management System

## Objective
Build a scheduled, automation-first drift-control system around one primary platform: RHACM or Argo.

## Target workflow
1. Detect manual cluster-side changes that diverge from Git-repo intended state.
2. Qualify drift and open a best-guess PR against the repo with the suspected changes.
3. Roll back the live cluster changes to avoid persistent drift.
4. Give reviewers enough time to complete and merge the PR before the automated rollback executes.

## Operational requirements
- recurrent schedule with immutable detection -> PR -> rollback phases
- retention of rollback grace period
- evidence package in each PR: diff source, affected manifests, cluster target, detection timestamp
- audit trail for who made the manual change and what the rollback did

## Constraints
- Git is source of truth; live cluster is not allowed to become authoritative by accident
- avoid recursive remediation loops
- manual changes are expected and treated as signals, not errors
- rollback must be reversible and safe to run on a schedule

## Design preferences
- one owner platform, not a hybrid split
- PR-first recovery path
- automation should be conservative and inspectable
