---
created_at: 2026-06-27
updated_at: 2026-06-27
owner: SamPlaysKeys
topic: drift-management-rhcm-argo
type: Type
status: exploring
review_status: awaiting_review
agent_generated: true
last_modified_by: agent
tags:
  - adr
  - argo
  - rhacm
---

# ADR — Choose Argo as the Drift Management Owner Platform

## Status
Proposed.

## Context
The drift management system must detect manual cluster-side changes, open a best-guess PR, and roll back drift on a schedule with a reviewer grace period. Two candidate owner platforms are Argo and RHACM.

## Decision
Use Argo as the primary owner platform for drift detection, PR generation, and rollback automation.

## Rationale
- Drift originates from the Argo Application layer, so Argo already models intended state and live state relationship directly.
- App-of-apps patterns make Git-repo comparison harder, but a test Argo renderer can produce the rendered manifests and provide a stable comparison baseline.
- Argo diff, sync, and rollback semantics already support the lifecycle we need without building a parallel control plane.
- RHACM excels at multi-cluster policy compliance and governance at scale, but adds an extra translation layer before PR creation and rollback.
- A single-platform design keeps automation logic centralized and reduces the surface area for permission and timing bugs.

## Alternatives Considered
- RHACM as owner would broaden coverage across managed clusters but complicate Git-to-PR mapping and require custom remediation adapters.
- Dual-platform ownership would split detection and enforcement, increasing the chance of recursive or conflicting remediation.

## Consequences
- The system must render app-of-apps manifests through a test Argo instance to obtain intended state.
- Rollback will be implemented via Argo deployment history or sync to the last known good commit.
- Reviewer grace periods and rollback safety rules must be enforced in automation above Argo.
