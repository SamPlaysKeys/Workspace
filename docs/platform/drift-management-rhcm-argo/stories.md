---
type: Note
_organized: true
---

# EPIC Stories — Argo/RHACM Drift Management

## Naming / sizing convention used
- Title: verb + noun + evidence/decision
- Length: small enough to ship within a few hours, with testable completion
- Linked to Feature: `ARGO-RHACM-DRIFT` (Feature)

---

## Story 1 — Drift event source normalization

**ID:** `ARGO-RHACM-DRIFT.1`
**Type:** Story
**Priority:** High
**Estimate:** 3
**Sprint:** 1

**As an** operator
**I want** ArgoCD sync status, RHACM policy compliance, and manual annotations to stream into one normalizer
**so that** downstream classification sees a consistent event shape

### Description
Create a single event bus or aggregation layer where each source maps to the same struct. Normalizer should:
- accept ArgoCD sync/health signals,
- accept RHACM policy compliance events,
- accept operator-issued manual annotations.

### Acceptance Criteria
- Normalizer emits `{timestamp, source, id, target, observed state, desired state, metadata}`.
- Producers and consumers are decoupled.
- Producers include at least Argo and RHACM adapters.

### Definition of Done
- Adapters run in test against fixture events.
- Output schema is versioned.
- Acceptance test reproduces an Argo + RHACM dual-source event.

---

## Story 2 — Drop-rule classification retention policy

**ID:** `ARGO-RHACM-DRIFT.2`
**Type:** Story
**Priority:** High
**Estimate:** 2
**Sprint:** 1

**As a** platform engineer
**I want** a durable drop-rule classifier with explicit retention/catch-all policy
**so that** drift classification is auditable and predictable when no rule matches

### Description
Implement classifier evaluator with:
- ordered rules evaluating event metadata,
- explicit catch-all when no match (audit trail required, never silently accept).

### Acceptance Criteria
- Each classified event records: matched rule id / unmatched, classification, timestamp.
- unmatched events are flagged and surfaced to operator review.
- Retention policy enforces 90 days in event store with eviction tests.

### Definition of Done
- Runbook entry documents ruleset update procedure.

---

## Story 3 — Operator drift dashboard (read-only)

**ID:** `ARGO-RHACM-DRIFT.3`
**Type:** Story
**Priority:** High
**Estimate:** 5
**Sprint:** 2

**As an** operator
**I want** a single pane showing drift state across managed clusters
**so that** I can orient on unplanned variance without querying each tool separately

### Description
Lightweight operator view summarizing:
- open drift events per cluster,
- classification breakdown (benign / action-required / unknown),
- link to originating Argo/RHACM source detail.

### Acceptance Criteria
- Data comes from previously normalized drift event store.
- View is read-only in Phase 1.
- Drill-through goes to ArgoCD UI and RHACM policy detail when available.

### Definition of Done
- Works from Tailscale operator network.
- No secrets in dashboard URLs.

---

## Story 4 — Manual remediation on-ramp

**ID:** `ARGO-RHACM-DRIFT.4`
**Type:** Story
**Priority:** High
**Estimate:** 5
**Sprint:** 2

**As an** operator
**I want** a structured remediation workflow for action-required drift
**so that** fixes are repeatable and recorded

### Description
On-ramp provides:
- a remediation checklist tied to drift class,
- a pre-filled remediation record template (proposed action, rollback hint, owner),
- outcome recording after remediation.

### Acceptance Criteria
- Each action-required drift is aware of its rollback path before execution.
- remediation decision is recorded whether the operator proceeds, defers, or marks accepted drift.
- Blocklist maintains list of drift categories not eligible for auto-remediation.

### Definition of Done
- Admin can demonstrate the flow end-to-end on a synthetic drift event.

---

## Story 5 — Evidence + rollback rehearsal integration

**ID:** `ARGO-RHACM-DRIFT.5`
**Type:** Story
**Priority:** Medium
**Estimate:** 3
**Sprint:** 3

**As a** platform engineer
**I want** drift event evidence packaged with rollback guidance
**so that** post-incident review and recovery are fast and deterministic

### Description
Link each drift event to:
- relevant Argo sync histories,
- relevant RHACM policy evaluation outputs,
- suggested rollback or sync command, if available.

### Acceptance Criteria
- Operator can export a drift event bundle for review.
- Rollback commands require explicit confirmation before execution.

### Definition of Done
- Drill procedure documented and tested against a real staged drift event.

---

## Story 6 — Phase 2 readiness gate

**ID:** `ARGO-RHACM-DRIFT.6`
**Type:** Story
**Priority:** Medium
**Estimate:** 2
**Sprint:** 3

**As a** platform engineer
**I want** an explicit Phase 2 readiness gate tied to Phase 1 acceptance outcomes
**so that** automation is not introduced until baseline control, observability, and safety are verified

### Description
Deliver:
- gated checklist derived from Phase 1 acceptance criteria,
- recorded Phase 1 outcomes as evidence,
- feature flag controlling Phase 2 auto-remediation entry.

### Acceptance Criteria
- Phase 2 cannot be enabled until all Phase 1 criteria are marked met.
- Evidence is stored alongside the feature definition.

### Definition of Done
- Evidence bundle reviewed and committed into project docs.

---

## Traceability
- All stories above derive from the Feature acceptance criteria.
- Each story references only work traceable to the Feature scope.
