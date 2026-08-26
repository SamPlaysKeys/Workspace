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

# ADR: Tailnet-per-environment (Prod / Test / Dev) with declarative cross-tailnet sharing

## Status

**Draft — heavy review expected.** This is a decision *framework*, not a settled decision. Sections marked **OPEN** need the human to pick a side before this moves to Proposed.

## Context

Today the homelab runs a **single tailnet** with tag-based environment separation. Per [tailscale-grants.md](../../network/tailscale-grants.md), isolation between Prod/Test/Dev is enforced entirely by grant rules:

```json
{"src": ["tag:prod"], "dst": ["tag:prod"], "ip": ["*:*"]}
{"src": ["tag:test"], "dst": ["tag:test"], "ip": ["*:*"]}
{"src": ["tag:dev"], "dst": ["tag:dev"], "ip": ["*:*"]}
```

That works, but the isolation is **one policy-file edit deep**. A bad merge to the policy file, a mis-tagged node, or a fat-fingered `tag:*` grant collapses Prod/Test/Dev into one flat network. The blast radius of a policy mistake is the entire homelab.

Two Tailscale changes make a different shape viable:

1. **Tailscale API / Services GA (Feb 2026)** — Services are now first-class, with declarative JSON config on-node, per-service audit logs, ACL tests, and API-driven lifecycle. Environment config becomes something Ansible/Komodo can reconcile rather than something clicked in a console.
2. **Declarative sharing / grants + `autogroup:shared`** — cross-tailnet access is expressible in the policy file rather than a pile of one-off invite links.

The question this ADR frames: **is a tailnet boundary the right isolation primitive for the homelab's environments, and is the cross-tailnet seam cheap enough to live with?**

## Drivers

| Driver | Weight | Notes |
|--------|--------|-------|
| Blast radius of a policy mistake | High | Current model: one file protects everything |
| Prod stability during Dev experiments | High | Dev is where breakage is *expected* |
| Operational overhead (per-tailnet admin, keys, DNS) | High | Homelab has one operator |
| Partner / shared access model | Medium | Adversarial access model already documented |
| Komodo GitOps flow across environments | Medium | Controller currently reaches all three |
| Observability / audit clarity | Medium | Per-tailnet audit logs are cleaner than tag filters |
| Cost | Low–Medium | **OPEN:** plan/seat implications of 3 tailnets |

## Options

### Option A — Status quo: one tailnet, tag-based separation

- **Pros:** zero migration; MagicDNS is flat and simple; Komodo controller reaches everything; one policy file to reason about; single admin console.
- **Cons:** isolation is soft (policy-only); one bad grant flattens all environments; no per-environment audit boundary; `tag:*`-style rules are easy to write and hard to spot in review.

### Option B — Three tailnets (Prod / Test / Dev), sharing only where required

- **Pros:** isolation is structural, not advisory — a Dev policy mistake cannot reach Prod; per-tailnet audit logs; per-tailnet auth keys and tagOwners; Dev can be treated as genuinely disposable.
- **Cons:** every cross-environment flow becomes a share; **shares strip tags, groups, and subnet info**; shared machines are **quarantined by default** (inbound only); shares are **user-scoped, not tag-scoped** — a tagged node cannot accept a share; three admin consoles; three sets of keys to rotate; MagicDNS names become tailnet-qualified FQDNs.

### Option C — Two tailnets: Prod isolated, Test+Dev share a lower tailnet

- **Pros:** captures most of the blast-radius win (Prod is structurally separate) at two-thirds the overhead; Test↔Dev promotion stays cheap and in-tailnet.
- **Cons:** Test no longer has a hard boundary from Dev, so "does Test mirror Prod?" gets fuzzier; still pays the cross-tailnet seam cost for the Prod promotion step.

### Option D — One tailnet, but harden the current model

Policy-file CI (ACL tests, `tailscale policy test` in GitHub Actions), mandatory review on the policy file, no `tag:*` destinations, and per-environment Services with their own grants.

- **Pros:** near-zero migration; keeps flat DNS; ACL tests convert "soft isolation" into "tested isolation."
- **Cons:** still one flat trust domain; a mistake that passes tests still lands everywhere; doesn't improve the audit boundary.

## Decision

**OPEN.** Recommended starting position for review: **Option C**, with Option D's policy CI adopted regardless of which option wins, because tested policy is cheap and useful in every branch of this tree.

Rationale for the lean: the blast-radius problem is really a *Prod* problem. Test and Dev breaking each other is annoying; Dev reaching Prod is the failure that matters. Option C buys the boundary that matters and skips the third console.

## The cross-tailnet seam — the part that decides this

Every option except A/D pays this cost, so it needs to be settled before the decision is:

1. **Tags don't cross.** Shares strip tags. Any grant that currently reads `tag:prod → tag:prod` has no cross-tailnet equivalent — the far side sees a user identity, not a tag. Cross-environment rules must be written against user identity or `autogroup:shared`.
2. **Machines can't accept shares — users can.** Machine-to-machine automation across tailnets (Komodo controller → Prod periphery agents) does **not** fit the sharing model. **OPEN:** either the controller gets a node in each tailnet, or each tailnet gets its own controller, or cross-tailnet reconciliation goes through a Service rather than a share.
3. **Quarantine is inbound-only by default.** Bidirectional flows need mutual sharing. Anything that needs to *initiate* outward (log shipping, metrics push, backup targets) needs explicit design.
4. **DNS gets longer.** Shared machines are reachable only as `<host>.<tailnet>.ts.net`. Every doc, bookmark, Komodo stack, and Compose file referencing a short MagicDNS name breaks.
5. **Services may be the better seam than sharing.** Services GA gives stable names, remote-destination proxies, and declarative JSON reload — which looks like a cleaner cross-boundary contract than sharing individual machines. **OPEN:** validate whether a Service can be published across a tailnet boundary in the way this design needs; if it can, sharing becomes a fallback, not the mechanism.

## Cross-environment flows to inventory before committing

Each of these currently works because everything is one tailnet. Each needs an explicit answer under any multi-tailnet option:

- Komodo controller (NUC, `tag:admin`) → Prod/Test/Dev periphery agents
- Promotion path DevDocker → Test → Prod (per [ADR-0001](0001-komodo-resourcesync-branch-per-environment.md), promotion is a file move; the network path is currently implicit)
- Observability scrape/push paths across environments
- Backup flows to `tag:storage` NAS devices (UnRaid = Prod, Synology = Test)
- Admin access from the operator's laptop/phone to all three
- Exit node — which tailnet owns it, or one per tailnet
- Partner/shared user access (currently modeled adversarially; may get *simpler* under Option B/C)
- ProxMox host, which hosts the Dev VM but is itself infrastructure

## Trade-offs accepted (if a multi-tailnet option is chosen)

- Longer FQDNs and a documentation sweep to match.
- Cross-environment automation is deliberately harder — that's the point, but it is a real ongoing tax.
- More secrets to rotate (auth keys, API keys per tailnet).
- Some current grant rules have no direct translation and must be redesigned rather than ported.

## Consequences / follow-on work

- Rewrite [tailscale-grants.md](../../network/tailscale-grants.md) per-tailnet; tag taxonomy shrinks per tailnet since environment identity moves into the boundary itself.
- Update [tailscale.md](../../network/tailscale.md) — Services naming, Docktail behavior, exit node strategy, access tiers.
- Add policy-file CI with ACL tests regardless of outcome.
- Decide where tailnet policy files live in Git and how they reconcile (API push vs. manual apply) — mirrors the ResourceSync branch question from ADR-0001.
- Migration must be reversible: no environment moves without a documented rollback to the single-tailnet state.

## Open questions

1. Plan/cost implications of 2–3 tailnets for a single-operator homelab.
2. Can Services span a tailnet boundary cleanly? If yes, does that make Option B cheap enough to beat C?
3. How does Komodo reconcile across a tailnet boundary — one controller with a node per tailnet, or a controller per tailnet?
4. Does Test lose meaning as a Prod rehearsal if it shares a tailnet with Dev (the Option C objection)?
5. Migration order — Dev first (lowest risk, proves the seam) or Prod first (proves the boundary that matters)?
6. Does the partner access model get simpler or just relocate?
