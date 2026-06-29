---
type: README-Note
created_at: 2026-06-27
updated_at: 2026-06-27
owner: SamPlaysKeys
topic: drift-management-rhcm-argo
status: exploring
review_status: awaiting_review
agent_generated: true
last_modified_by: agent
tags:
  - docs
  - platform
  - drift-management
---

# Drift Management System — Rhcm/Argo

## Overview

This project defines a schedule-plus-grace-term model for detecting configuration drift in Argo-managed clusters, converting that drift into reviewable pull requests, and restoring live state if the changes are not accepted within a configured reviewer window.

The system does not replace standard GitOps review flows. It augments them by reducing the window of risk between a human mistake on the cluster and its reconciliation back into Git.

## Problem

Manual changes in Argo-managed clusters create persistent drift. When an operator edits a resource directly in a cluster, the desired state in Git no longer matches live state. Until that drift is captured, reviewed, and rejected or accepted, the source of truth is split across two systems.

## Goals

- Detect manual cluster-side changes with low noise.
- Open a best-guess PR containing the suspected changes and enough context to review.
- Grace the reviewer with a configurable window before automatic rollback executes.
- Make the loop schedule-driven and idempotent.
- Target changes at the appropriate child application and repository, even in app-of-apps hierarchies.

## Non-Goals

- Replacing standard GitOps review workflows.
- Building a new configuration-management controller.
- Preventing manual changes in real time.

## Architecture Overview

Heart of the system is a renderer plus normalizer plus diff classifier plus PR author plus rollback scheduler pipeline. The renderer uses a dedicated test Argo instance to obtain the intended state for comparison. The normalizer strips non-deterministic metadata from both intended and live manifests so they remain comparable byte-for-byte. The classifier assigns risk and grace period rules based on resource kind, file path, and namespace. The PR author writes changes back to Git; the rollback scheduler reconciles live state if the PR stays unmerged past its grace window.

## Component Roles

| Document | Role |
|----------|------|
| [design.md](design.md) | Architecture and data flow. |
| [normalizer-contract.md](normalizer-contract.md) | How rendered and live manifests are shaped into comparable, deterministic outputs. |
| [process-plan.md](process-plan.md) | Implementation steps and validation plan. |
| [adr-app-of-apps-child-dispatch.md](adr-app-of-apps-child-dispatch.md) | ADR for rendering and dispatching at the child Application level. |
| [adr-argo-as-owner.md](adr-argo-as-owner.md) | ADR for choosing Argo over RHACM as the primary owner platform. |

## Timing Model

The system uses a schedule-plus-grace-term model.

- Detection and PR opening occur on a schedule.
- The grace term begins when the PR opens.
- A pre-rollback gate checks whether the PR is merged, closed as not planned, or marked do-not-rollback.
- Durations are configurable per app and per risk class.

## Safety Model

- Rollback is opt-out by label or classifier decision, not opt-in.
- All rollbacks are reversible from Argo deployment history.
- Observability hooks emit audit log events for detection, PR creation, and rollback.

## Risks Worth Highlighting

- MutatingWebhook annotations or controller churn can still create noise.
- Cross-namespace promotion scenarios must be handled carefully because identity hashing includes namespace.
- App-of-apps hierarchies add an extra layer of rendered manifests that affects pairing and diff scope.
- The `argocd-diff-preview` integration should be validated on live manifests before operating on production clusters.

## Open Questions

- Exact PR schema and classifier rules for the first release.
- Identity and permissions for PR creation and cluster rollback.
- Alerting channels and escalation paths for rollback events.
- How to handle multiple drift events per application per run.

## Status

This document set was promoted from a storm session and is awaiting review.
