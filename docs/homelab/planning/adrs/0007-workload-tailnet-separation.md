---
type: ADR
Status: Proposed
Date: 2026-08-26
agent_generated: true
agent_at: "2026-08-26T00:00:00Z"
review_status: "awaiting_review"
reviewed_by: ""
last_modified_by: "agent"
---

# ADR: Workload isolation in a single tailnet — access pools

## Status

**Proposed.** Direction settled: workload/dev traffic stays in the current tailnet, separated by ACLs organized as **access pools**. The separate-tailnet option is deferred to a named trigger, not discarded.

Extends **[ADR-0006](0006-tailnet-per-environment.md)**, which rejected splitting Prod/Test/Dev into separate tailnets. That investigation's useful finding was that the boundary axis was wrong, not the boundary idea. This ADR takes the next step of that finding: separation of workload from homelab is real, but the mechanism is **pools within one boundary**, with structural separation preserved as an off-ramp.

## Context

Two distinct things are currently conflated inside one tailnet:

1. **The homelab** — Prod and Test infrastructure, private applications, the Komodo GitOps pipeline, storage, admin interfaces. Everything here is either operator-only or shared with the partner as a trusted user.
2. **Dev** — nominally a homelab environment, but in practice where non-homelab work happens. It has no promotion relationship worth preserving with Test/Prod for that class of work.

The earlier draft of this ADR weighed two mechanisms: a **separate tailnet** for workload (structural isolation, independent policy, clean audit boundary) against staying in the **shared tailnet** with tag separation (zero migration, isolation only advisory). The pivotal consideration — that a boundary's alpha dependency sits on a path that is allowed to degrade, unlike ADR-0006's admin/Komodo paths — is retained.

The judgment has since shifted on the trigger. The conditions that would justify a boundary — outside resources managing elements, sustained workload or external-party growth — are not real today. Building the structural split now pays complexity for a hypothetical, and it puts reachability that does not yet need to exist on an alpha, waitlist-gated feature (declarative node sharing). The boundary is kept as the off-ramp, priced and designed, but not built.

## Decision

**Single tailnet, workload separated by access pools. The structural split is deferred, not discarded.**

### 1. One tailnet for now

Workload/Dev elements stay in the current tailnet, protected by ACLs rather than a boundary. Nothing new to provision, no per-tailnet settings (`httpsEnabled`, tagOwners, auth keys), and the Services/Docktail/ScaleTail exposure layer works exactly as today. External sharing, when it happens, stays **link-based** — one machine handed to one invited user — which covers ad-hoc guest access without committing to a separate tailnet.

### 2. Scope framing shifts from "homelab first" to "environment with pools"

The tailnet is no longer scoped as the homelab with everything else grafted on. It is the **environment** housing purpose-defined pools — provided services as one pool, development/workloads as another. The pool set is open: additional pools are named when a need clears the bar; none are pre-declared here (see Open Questions). This reframing drives everything that follows: nodes are organized by **purpose AND access**, and the ACL question becomes *"which pools can which users and services reach."*

### 3. ACLs are pool-based, not environment-tag-based

The access model follows the pools: grants are written in terms of who may reach which pool, in which direction. Two concept principles, without enumerating the pools themselves:

- **One pool per node.** A node belongs to exactly one pool. Cross-pool reach is an explicit, named grant — never a second membership. This keeps the ACL table non-ambiguous: a node's pool is its identity, and exceptions are visible rules rather than overlapping tags.
- **Directional by default.** Workload → provided services is the natural direction and the common grant (dev needs to reach the things the environment provides). Services → workload is a named exception, not part of the default posture. Reverse-direction access is the thing that erodes the separation, so it must always be a visible rule.

The pool taxonomy itself — names, membership, concrete grants — is deliberately unspecified in this ADR. It is a follow-on design task (see Consequences).

### 4. Separate tailnet = deferred off-ramp

If the environment grows and **outside resources need to manage elements**, the workload moves to a **new set of devices in a separate tailnet** — the homelab tailnet untouched. The earlier draft's separate-tailnet analysis is retained as the off-ramp design: structural isolation, independent policy file, clean audit boundary, external-group sharing without exposing private applications. Its alpha dependency (declarative node sharing) is acceptable *once the boundary is real*, because it then sits on a workload path that is allowed to degrade — the same risk posture, just correctly timed.

The pool taxonomy chosen now is the seam that cut will follow: pools map cleanly to future tailnet membership, and a split becomes a placement/policy exercise rather than a redesign. Policy-file CI should be in place first — a split that is mostly policy movement is only cheap if the policy is already tested.

## Why this succeeds where ADR-0006 failed

| Concern | ADR-0006 (environment split) | This ADR (pools) |
|---|---|---|
| Admin/Komodo paths | Cut by the boundary — homelab manageability behind an alpha feature | Untouched; all stay in the homelab tailnet |
| Existing Services layer | Rewritten across three tailnets | Untouched |
| Partner/guest access | Complicated by tailnet-qualified DNS | Link-based sharing, unchanged from today |
| Alpha dependency | On the critical management path | On the deferred off-ramp only |
| Migration cost | Three tailnets, full grants rewrite | Zero now; the split is a future, triggered exercise |
| Blast radius | Hypothetical (bad grant) | Contained within pool grants — and CI-testable |

## Future option: separate tailnet (deferred, priced)

**Noted, not built.** When the trigger fires — outside resources managing elements, or sustained external-party growth — execute the off-ramp: new device set in a separate tailnet, provisioned via the API chain (OAuth exchange → create tailnet → `httpsEnabled` → tagOwners/policy → scoped auth keys), homelab tailnet untouched, cross-boundary access via declarative node sharing. Joining the waitlist for declarative sharing is *only* needed when the trigger becomes real; doing it now starts a clock for a feature nothing active depends on.

## Open questions

1. **Pool taxonomy** — deliberately unspecified here. The concrete pools, names, membership, and grants are the follow-on design task. Implicit constraint from this ADR: no "miscellaneous" pool; a catch-all pool is how pools rot into a flat network.
2. **Does the partner need any workload access?** Default assumption: no — workload pools are operator-only. Link-based sharing remains available per-need; the answer affects whether the future split needs external-group modeling at all.
3. **Exit node** — does workload need its own, or use none?
4. **Split trigger threshold** — what concretely constitutes "outside resources managing elements"? Naming the threshold now makes the off-ramp decision unemotional later.
5. **Quarantine semantics / synced groups** — relocated to the off-ramp; only apply once a boundary exists.

## Consequences / follow-on work

- **Design the pool taxonomy and draft the HuJSON policy** — tracked TODO, not done here. A policy skeleton with the pool structure and directional grants, reviewed against the principles above.
- **Policy-file CI with ACL tests** — now *more* load-bearing than under either prior framing: pools keep isolation advisory, and CI is what converts it into *tested* isolation. Ban `tag:*` destinations, require review on the policy file, map pool grants to tests, run in GitHub Actions.
- **Update [tailscale-grants.md](../../network/tailscale-grants.md)** once the pool taxonomy is concrete; convert the environment-tag framing to pool framing.
- **Update [tailscale.md](../../network/tailscale.md)** to document workload pools, link-based sharing for guests, and the deferred-split framing.
- **Do not join the declarative-sharing waitlist yet** — retracted from the earlier draft; only needed once the off-ramp trigger is real.
- **Rename Dev → Workload** in homelab docs, and be explicit that it is outside the promotion chain, so it isn't mistaken for a pipeline stage.
- **Revisit [ADR-0001](0001-komodo-resourcesync-branch-per-environment.md)**: if Dev leaves the promotion chain, the per-environment ResourceSync branch layout has one fewer environment to steer.

## References

- [ADR-0006: Tailnet-per-environment](0006-tailnet-per-environment.md) — the rejected environment split; contains the link-based vs. declarative sharing mechanism comparison and the API provisioning details
- [Declarative node sharing](https://tailscale.com/docs/features/declarative-node-sharing) — alpha, waitlist-gated; the off-ramp's cross-boundary mechanism, not active today
- [Share your machines with other users](https://tailscale.com/docs/features/sharing) — link-based sharing; the active mechanism for external access under this ADR
- [summer-with-tailscale](https://github.com/frozenprocess/summer-with-tailscale) — worked examples for the API-driven tailnet provisioning chain the off-ramp will use
- [tailscale.md](../../network/tailscale.md) — current Services, Docktail/ScaleTail, and access-tier design
- [tailscale-grants.md](../../network/tailscale-grants.md) — current tag taxonomy and grant rules