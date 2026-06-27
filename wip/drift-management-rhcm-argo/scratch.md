---
created_at: 2026-06-27
owner: SamPlaysKeys
topic: drift-management-rhcm-argo
type: Note
status: exploring
review_status: awaiting_review
agent_generated: true
last_modified_by: agent
---

# Scratch Notes — RHACM / Argo Drift Management

## Useful references
- https://github.com/dag-andersen/argocd-diff-preview — supports app-of-apps; currently used for two-desired-state PR diffs
- This tool is the likely candidate for live-vs-rendered diff if its diff semantics can be applied to live normalized manifests
- RHACM documentation for drift detection strategies
- ArgoCD Application diff and sync semantics
- GitOps conflict and safeguard patterns

## Collected signals
- Drift management systems benefit most when detection, classification, and remediation are separated.
- Tooling that writes back to Git reduces attribution errors but requires clear ownership.
- Review-first workflows prevent over-automation in small operator teams.

## Possible directions
- Start with detection + alerting; add remediation after signal quality is trusted.
- Extend an existing pipeline instead of building a dedicated new controller.
- Model the system as policy-first so drift rules can evolve without changing control-plane code.
