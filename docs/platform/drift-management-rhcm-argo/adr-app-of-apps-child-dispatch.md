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
  - app-of-apps
  - drift-detection
---

# ADR — Use Hybrid Root Discovery plus Child Dispatch for App-of-Apps Drift

## Status
Proposed.

## Context
The drift management system must detect cluster-side drift in an app-of-apps Argo layout and open a best-guess PR against the appropriate Git repository. Three options were considered for mapping the top-level app-of-apps structure to drifted child applications: flatten and ignore hierarchy; trace child `Application` manifests and render each separately; or hybrid root discovery plus child dispatch.

## Decision
Use hybrid root discovery plus child dispatch.

## Rationale
- The top-level app-of-apps render only defines which child `Application` resources exist; meaningful configuration drift lives in the child sources.
- Child dispatch lets the system target the correct repository for each drift event, which keeps PRs actionable.
- Root discovery avoids blind rendering of every possible child source on every run; only live child apps are evaluated.
- This approach naturally supports an app-level lifecycle: detect, classify, PR, grace term, and rollback scoped to the child app instead of the whole tree.
- It reduces noise and rendering cost compared with full-tree flattening, and it avoids the “wrong repo” failure mode of a flat diff.

## Alternatives Considered
- Flatten and ignore hierarchy: simplest, but breaks repository targeting and review ownership.
- Trace child manifests from root render and render every child: correct ownership, but forces broad rendering on every run and adds unnecessary work for inactive or unchanged children.

## Consequences
- The test Argo renderer is used at two levels: root discovery and child intended-state capture.
- The scheduler must maintain per-child render identity and diff results across runs.
- Child `Application` CRD drift in the root repo still needs its own detection and PR path.
