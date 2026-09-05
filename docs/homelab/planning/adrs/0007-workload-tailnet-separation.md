---
type: ADR
Status: Draft
Date: 2026-08-26
agent_generated: true
agent_at: "2026-08-26T00:00:00Z"
review_status: "awaiting_review"
reviewed_by: ""
last_modified_by: "agent"
---

# ADR: Workload tailnet separation — repurposing Dev for non-homelab work

## Status

**Draft.** The direction is settled; the mechanism is not. Sections marked **OPEN** need a decision before this moves to Proposed.

Follows from **[ADR-0006](0006-tailnet-per-environment.md)**, which rejected splitting Prod/Test/Dev into separate tailnets. That investigation's useful finding was that the boundary axis was wrong, not the boundary idea.

## Context

ADR-0006 established that imposing a tailnet boundary between homelab *environments* is expensive and unhelpful: it cuts across paths that need to stay open (Komodo controller → periphery agents, admin access, observability, backups) and rewrites a Services exposure layer that already works.

The boundary that *does* pay for itself runs along a different seam: **homelab vs. non-homelab work.**

Two distinct things are currently conflated inside one tailnet:

1. **The homelab** — Prod and Test infrastructure, private applications, the Komodo GitOps pipeline, storage, admin interfaces. Everything here is either operator-only or shared with the partner as a trusted user.
2. **Dev** — nominally a homelab environment, but in practice where non-homelab work happens. It has no promotion relationship worth preserving with Test/Prod for that class of work.

Unlike the environment split, a boundary here does not cut any path that needs to stay open. Non-homelab workloads have no legitimate reason to reach Prod infrastructure, and Prod has no reason to reach them.

## Decision

**Two changes, one settled and one open.**

### 1. The homelab tailnet is the private-application tailnet — settled

The existing tailnet keeps its current structure: tag-based Prod/Test separation, existing Services and Docktail/ScaleTail exposure, existing grants, existing admin tiers. **All private applications live here, and the partner is included in this group as a trusted user** — this is home infrastructure and the people who live in the home.

This supersedes the framing in [tailscale.md](../../network/tailscale.md) that treats partner access purely adversarially at the *tailnet* level. **OPEN:** the adversarial model still has value at the *service* tier (admin portals, SSH, subnet routes stay operator-only). What changes is that partner membership in the private tailnet is now the intended design rather than an exception carved out with shares. That distinction needs writing up properly in `tailscale.md`.

### 2. Dev becomes the Workload environment — settled in purpose, open in mechanism

Dev is repurposed as **Workload**: where non-homelab work happens. It is explicitly **not** an environment in the Prod/Test promotion chain, and its relationship to Test/Prod is "unrelated," not "upstream."

**OPEN — the mechanism:**

| Approach | Pros | Cons |
|---|---|---|
| **Separate tailnet** for Workload | Structural isolation from homelab; independent policy file, tags, and auth keys; workload experiments can't touch home infrastructure; clean audit boundary; supports sharing to external groups without exposing the homelab tailnet | Another tailnet to provision and administer; per-tailnet `httpsEnabled` and tagOwners; cross-boundary access needs declarative node sharing (**alpha, waitlist-gated**); depends on the unresolved Services-across-boundary question |
| **Shared tailnet** (stay in current tailnet, separated by tags) | Zero migration; nothing new to operate; Services work exactly as today | Isolation stays advisory — the same soft-boundary critique ADR-0006 leveled at environment tags; a workload mistake can reach home infrastructure; no audit separation; external-group sharing means sharing *from* the tailnet holding private apps |

The pivotal difference from ADR-0006: here the alpha dependency sits on a path that is **allowed to be degraded**. If declarative sharing breaks or changes, workload access to homelab resources suffers — homelab manageability does not. That is an acceptable risk posture, and it is why a separate tailnet is viable now rather than "once it leaves alpha."

**Leaning:** separate tailnet, if the external-group sharing requirement is real. Sharing non-homelab work to outside parties from the same tailnet that holds private applications is the specific thing worth structurally preventing.

## Why this succeeds where ADR-0006 failed

| Concern | ADR-0006 (environment split) | ADR-0007 (workload split) |
|---|---|---|
| Admin/Komodo paths | Cut by the boundary — homelab manageability behind an alpha feature | Untouched; all stay in the homelab tailnet |
| Existing Services layer | Rewritten across three tailnets | Untouched for homelab; only Workload is new |
| Partner access | Complicated by tailnet-qualified DNS | Simplified — partner is a member of the private tailnet |
| Alpha dependency | On the critical management path | On a degradable path only |
| Migration cost | Three tailnets, full grants rewrite | One new tailnet at most; homelab unchanged |
| Blast radius solved | Hypothetical (bad grant) | Concrete (external sharing, workload experiments) |

## Future option: collapse three environments to two

**Noted, not decided.** With Dev repurposed as Workload, the homelab is left with Prod and Test. The Prod/Test/Dev tag taxonomy in [tailscale-grants.md](../../network/tailscale-grants.md) could collapse to **Test and Prod only** — `tag:dev` retires from the homelab tailnet, and the Dev-specific nodes (DevDocker VM, OCP cluster) get reassigned or move to Workload.

This is deliberately out of scope. It touches [ADR-0001](0001-komodo-resourcesync-branch-per-environment.md) (per-environment ResourceSync branches), [ADR-0002](0002-baremetal-host-os.md) (host OS per environment), and the three-tier managed-host decision in [decisions.md](../decisions.md). It deserves its own ADR once Workload has settled and it is clear what Dev was actually still being used for.

## Open questions

1. **Separate tailnet or shared?** The central decision. Hinges largely on whether external-group sharing is a real near-term requirement or a hypothetical.
2. **Can a Service be published across a tailnet boundary?** Carried over from ADR-0006 and now load-bearing. If yes, workload apps can stay where they run and only exposure crosses. If no, they must physically live in the Workload tailnet — which means container placement, and possibly hardware, moves.
3. **Which nodes move?** DevDocker VM, the OCP cluster in Dev, ProxMox's role as host. ProxMox is homelab infrastructure but hosts workload VMs — does it stay wholly in the homelab tailnet?
4. **External groups — one Workload tailnet or one per group?** Per-group gives real tenant isolation between external parties; one shared costs less to operate. Only worth splitting if more than one external party is expected.
5. **Does the partner need any Workload access?** Default assumption: no. Worth confirming, since it's the difference between a clean boundary and a standing exception.
6. **Quarantine semantics** — do they apply to declaratively shared nodes, or does `allowIncomingConnections` fully replace them? Relocated from ADR-0006.
7. **Synced groups** (SCIM/Google) cannot be referenced across tailnets. Does any current or planned group source conflict? Relocated from ADR-0006.
8. **Does Workload need its own exit node**, or does it use none?
9. **Cost** — plan implications of an additional tailnet on the current account.

## Consequences / follow-on work

- Update [tailscale.md](../../network/tailscale.md): partner as a member of the private tailnet rather than an adversarial share; retain service-tier restrictions; document the Workload environment.
- Update [tailscale-grants.md](../../network/tailscale-grants.md) once the mechanism is chosen. Under the shared-tailnet option this is a tag and grants change; under separate-tailnet it is a new policy file plus `externalTailnets`.
- Rename Dev → Workload in homelab docs, and be explicit that it is outside the promotion chain, so it isn't mistaken for a pipeline stage.
- Revisit [ADR-0001](0001-komodo-resourcesync-branch-per-environment.md): if Dev leaves the promotion chain, the per-environment ResourceSync branch layout has one fewer environment to steer.
- If separate-tailnet is chosen: provisioning artifact in `artifacts/` per repo convention — OAuth exchange → create tailnet → `httpsEnabled` → tagOwners/policy → scoped auth keys. [`04-api-is-the-way/end-to-end.sh`](https://github.com/frozenprocess/summer-with-tailscale/blob/main/04-api-is-the-way/end-to-end.sh) is a working reference for that chain.
- Join the declarative node sharing waitlist now (admin console → General → Feature previews) — only the *sharing* tailnet needs it enabled, and access is granted by email on Tailscale's timeline, so the clock should start before the mechanism is finalized.
- Policy-file CI with ACL tests, carried forward from ADR-0006, applies regardless and becomes more valuable with two policy files.

## References

- [ADR-0006: Tailnet-per-environment](0006-tailnet-per-environment.md) — the rejected environment split; contains the link-based vs. declarative sharing mechanism comparison and the API provisioning details
- [Declarative node sharing](https://tailscale.com/docs/features/declarative-node-sharing) — alpha, waitlist-gated; `externalTailnets`, double opt-in, `tag://`/`group://` external references
- [Share your machines with other users](https://tailscale.com/docs/features/sharing) — link-based sharing; the right tool for handing a single machine to a single outside human
- [summer-with-tailscale](https://github.com/frozenprocess/summer-with-tailscale) — worked examples for API-driven tailnet provisioning and declarative sharing
- [tailscale.md](../../network/tailscale.md) — current Services, Docktail/ScaleTail, and access-tier design
- [tailscale-grants.md](../../network/tailscale-grants.md) — current tag taxonomy and grant rules
