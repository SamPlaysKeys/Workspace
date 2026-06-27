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
  - design-doc
  - candidate
---

# Candidate Design Document — Drift Management System

## Problem
Manual changes in Argo-managed clusters create persistent drift. The desired behavior is to convert that drift into a reviewable pull request, then restore live state before drift becomes the source of truth.

## Goals
- Detect manual cluster-side changes with low noise
- Open a best-guess PR containing the suspected changes
- Roll back cluster changes after a safe reviewer window
- Make the whole loop schedule-driven and idempotent

## Non-Goals
- Replacing standard GitOps review workflows
- Building a new configuration-management controller
- Supporting real-time prevention of manual changes

## Architecture Overview
- Owner platform: Argo
- Drift signal source: rendered test Argo instance diffed against live cluster state
- Recovery path: PR for human review, automated rollback if PR stays unmerged

## Component: Render Service
- Uses a dedicated test Argo instance to render app-of-apps manifests via Argo render API or equivalent
- Produces the intended state for comparison, bypassing app-of-apps indirection limits
- Triggers on schedule and on-demand
- Normalizer contract strips non-deterministic fields before comparison
- Output is normalized for diffing by stripping non-deterministic fields

## Component: Drift Detector
- Compares rendered test output to live cluster state
- Produces structured drift output: kind, namespace, name, field path, expected value, observed value
- Filters known benign drift sources to reduce noise

## Component: Drift Classifier
- Labels drift by risk and type from file paths and resource kinds
- Determines reviewer grace period and whether automatic rollback is permitted
- Can be extended without control-plane code changes

## Component: PR Author
- Creates or updates target branches for each drift event
- Generates commit messages and PR body from drift metadata
- Includes app, cluster, timestamp, and reproduction details

## Component: Rollback Scheduler
- Waits through the configured reviewer window
- Verifies PR state before rollback
- Reverts live cluster state using the prior known good commit or deployment history
- Emits audit log entries for detection, PR creation, and rollback

## Data Flow
1. Renderer produces intended manifests
2. Detector compares intended to live and writes drift report
3. Classifier assigns risk and grace period
4. PR Author opens or updates a PR
5. Rollback Scheduler checks PR status after grace period
6. If unmerged, rollback executes and is logged

## Timing model
- schedule-plus-grace-term model
- detection and PR opening happen together
- grace term begins when PR opens
- pre-rollback gate: PR merged, closed as not planned, or marked do-not-rollback
- configurable durations per app and per risk class

## Safety Model
- Rollback is opt-out by label or classifier decision, not opt-in
- All rollbacks are reversible from Argo deployment history
- Observability hooks emit events for detection, PR, and rollback phases

## Open Questions
- Exact diff format and normalization rules for resources
- How to handle multiple drift events per app per run
- Identity and permissions for PR creation and cluster rollback
- Alerting channel for rollback events

## Next Steps
- ADR selection: test Argo renderer as intended-state source
- Prototype render -> diff -> PR pipeline for one app
- Define PR schema and classifier rules for first release
