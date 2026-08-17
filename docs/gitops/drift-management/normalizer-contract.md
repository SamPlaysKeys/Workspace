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
  - diff-normalizer
  - contract
  - pipeline
---

# Diff Normalizer Contract

## Purpose
Convert live cluster manifests and rendered test manifests into equivalent, deterministic trees suitable for `argocd-diff-preview` or a standard structural diff. The normalizer removes non-deterministic and infrastructure-only fields, aligns formatting, and records explicit deletion semantics.

## Scope
- Input A: rendered manifests from test Argo instance
- Input B: live manifests from target cluster
- Output: normalized manifest trees on both sides
- Consumer: diff toolchain, classifier, PR generator

## Non-Goals
- Mutating live cluster state or writing back to Git
- Replacing Argo diff semantics; this is a normalization preprocessor

## Determinism Contract
All normalized manifests MUST:
- Exclude non-deterministic metadata fields
- Exclude infrastructure-only status and bookkeeping fields
- Preserve semantic differences relevant to configuration drift
- Be sortable and comparable byte-for-byte after normalization

## Stripped Fields

### Always stripped
- `metadata.resourceVersion`
- `metadata.uid`
- `metadata.creationTimestamp`
- `metadata.selfLink`
- `metadata.annotations["kubectl.kubernetes.io/last-applied-configuration"]`
- `metadata.annotations["autoscaling.alpha.kubectl.io/conditions"]`
- `metadata.annotations["autoscaling.alpha.kubectl.io/currentReplicas"]`
- `metadata.annotations["autoscaling.alpha.kubectl.io/previousReplicas"]`
- `metadata.annotations["autoscaling.alpha.kubectl.io/previousCPUUtilizationPercentage"]`
- `metadata.annotations["autoscaling.alpha.kubectl.io/currentCPUUtilizationPercentage"]`
- `status`
- `metadata.managedFields`
- `metadata.annotations["control-plane.alpha.kubernetes.io/leader"]`
- `metadata.annotations["deployment.kubernetes.io/revision"]`

### Stripped unless allowlisted
- `metadata.annotations[timestamp-*]`
- `metadata.annotations[sidecar.istio.io/*]`
- `metadata.annotations["kubectl.kubernetes.io/restartedAt"]`
- `metadata.annotations` set by admission webhooks or controllers

## Allowlist Mechanism
Operators MAY extend an allowlist for resources that intentionally require otherwise-stripped fields. Allowlist entries include:
- resource kind
- apiVersion
- annotation key glob or exact key
- reason for retention

## Ordering Rules
- Within each manifest, keys MUST be sorted alphabetically after normalization
- Within `metadata` and `spec`, preserve nested object boundaries but normalize key order
- Lists MAY be left in declaration order unless explicitly unstable

## Deletion Semantics
- A field present in desired but absent in live MUST be treated as a pending add
- A field present in live but absent in desired MUST be treated as a pending delete
- Resource deletion is represented by manifest absence in desired and presence in live

## Empty and Null Handling
- Empty maps and lists are normalized to presence if meaningful, otherwise stripped
- Null values are converted to absent fields

## Identity Hashing
- Each normalized manifest MUST include a stable identity key derived from `apiVersion`, `kind`, `namespace`, and `name`
- Used for pairing manifests across desired and live before diffing

## Validation Rules
- Output manifests MUST be valid YAML after normalization
- Normalizer MUST fail closed on parse errors
- Diff input MUST produce structured output with affected paths and expected versus observed values

## Extension Points
- New strip rules via configuration, not code changes
- Per-kind normalization hooks if new resource kinds exhibit non-deterministic behavior
