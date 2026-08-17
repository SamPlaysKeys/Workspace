---
type: Note
_organized: true
---

# EPIC: Argo/RHACM Drift Management — Feature Overview

**Project:** Homelab GitOps Platform
**Component:** Kubernetes / OpenShift Management
**Status:** Draft
**Priority:** Medium
**Sources:** wip/drift-management-rhcm-argo (BACKLOG), competence with RHACM + ArgoCD operational patterns

---

## Summary
Reduce unplanned configuration drift in managed Kubernetes/OpenShift environments by combining ArgoCD detection, RHACM policy signal, and operator-driven remediation. The feature is the end-to-end "detect → classify → on-ramp to remediation" experience in the homelab.

## Problem
Clusters managed by GitOps and RHACM can drift from cluster intent without anyone noticing quickly enough. Current state lacks:
- consistent drift context across Argo and RHACM signals,
- a predictable response path per drift class,
- and durable evidence for post-incident review.

## Goals (Jira Feature scope)
1. Unify ArgoCD sync drift and RHACM policy compliance into a single operator-facing view.
2. Classify drift as benign runtime variance or action-required.
3. Provide automated classification + human approval for remediation.
4. Capture drift evidence and response outcome for platform documentation.

## Non-goals
- Full closed-loop self-healing without human review.
- Unified auth, secrets handling, or enterprise SSO for this feature.
- Replacing existing GitOps workflow — drift management augments it.

## Acceptance Criteria
1. Operator can see latest drift state from one dashboard.
2. High-risk drift triggers separate approval gate from benign runtime drift.
3. Every drift event has: detected time, source (Argo/RHACM/manual), classification, remediation action, outcome.
4. Phase 1 rollback plan is documented and rehearsed.
5. Phase 2 must pass Phase 1 acceptance criteria before enabling auto-remediation.

## Security / Safety Constraints
- Do not auto-remediate persistent volume state, node counts, or partition attachment classes without explicit operator approval per event.
- Audit trail is not optional: retain 90 days of drift event history.

## Success Metrics
- Drift detection-to-classification latency under five minutes.
- blocker incidents caused by unknown drift reduced to zero within one quarter of GA.

## Phasing
- Phase 1: Detection, classification, operator dashboard, manual remediation on-ramp.
- Phase 2: Guided remediation automations, policy suppression/allowlisting, richer tooling integrations.
