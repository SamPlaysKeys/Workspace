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

# ADR: Tailnet-per-environment (Prod / Test / Dev) with declarative node sharing

## Status

**Draft — heavy review expected.** This is a decision *framework*, not a settled decision. Sections marked **OPEN** need a human call before this moves to Proposed.

## Context

Today the homelab runs a **single tailnet** with tag-based environment separation. Per [tailscale-grants.md](../../network/tailscale-grants.md), isolation between Prod/Test/Dev is enforced entirely by grant rules:

```json
{"src": ["tag:prod"], "dst": ["tag:prod"], "ip": ["*:*"]}
{"src": ["tag:test"], "dst": ["tag:test"], "ip": ["*:*"]}
{"src": ["tag:dev"], "dst": ["tag:dev"], "ip": ["*:*"]}
```

That works, but the isolation is **one policy-file edit deep**. A bad merge, a mis-tagged node, or a fat-fingered `tag:*` destination collapses Prod/Test/Dev into one flat network. The blast radius of a policy mistake is the entire homelab.

Three Tailscale capabilities change the calculus:

1. **Additional tailnets via the API.** `POST /api/v2/organizations/-/tailnets` provisions a fully isolated tailnet under the same account, no console clicks. The response carries a **tailnet-scoped OAuth client** — creation is org-scoped, but administration and deletion require that per-tailnet credential ([worked example](https://github.com/frozenprocess/summer-with-tailscale/blob/main/01-tailnet-sandboxes/readme.md)).
2. **Declarative node sharing** ([docs](https://tailscale.com/docs/features/declarative-node-sharing), **alpha, waitlist-gated**). Cross-tailnet access becomes a policy-file construct — `externalTailnets` plus grants referencing `group://<alias>/<name>` or `tag://<alias>/<name>`. It is explicitly designed for **machine-to-machine access between trusted tailnets**, which is exactly the homelab's shape.
3. **Tailscale Services GA (Feb 2026).** Stable service identities, declarative on-node JSON config, remote-destination proxies, per-service audit logs, and ACL tests.

The question this ADR frames: **is a tailnet boundary the right isolation primitive for the homelab's environments, and is the cross-tailnet seam cheap enough to live with?**

### Correction to an earlier framing

An earlier draft of this ADR analyzed the seam using **link-based sharing** ([sharing docs](https://tailscale.com/docs/features/sharing)) and concluded the model was largely unworkable — tags stripped, shares user-scoped so tagged machines can't accept them, quarantine by default. **Those constraints are real but belong to link-based sharing, not declarative node sharing.** Declarative sharing operates at the policy level and supports tag references and machine-to-machine access directly. The two mechanisms coexist; picking the wrong one produces the wrong conclusion, which is why they're separated explicitly below.

## Drivers

| Driver | Weight | Notes |
|--------|--------|-------|
| Blast radius of a policy mistake | High | Current model: one file protects everything |
| Prod stability during Dev experiments | High | Dev is where breakage is *expected* |
| Operational overhead (per-tailnet admin, keys, DNS) | Medium | Lower than assumed — API-drivable |
| Dependence on an alpha feature | High | Declarative sharing is alpha + waitlist |
| Komodo GitOps flow across environments | Medium | Controller currently reaches all three |
| Observability / audit clarity | Medium | Per-tailnet audit is cleaner than tag filters |
| Partner / shared access model | Medium | Adversarial model already documented |
| Cost | Low–Medium | **OPEN:** plan implications of additional tailnets |

## The two sharing mechanisms

Getting this distinction right is most of the decision.

| | Link-based sharing | Declarative node sharing |
|---|---|---|
| Status | GA (beta flag in docs), v1.4+ | **Alpha, waitlist-gated** |
| Unit of sharing | One machine → one user | Policy-level, resource classes |
| Tags across the boundary | **Stripped** | **Supported** via `tag://<alias>/<tag>` |
| Machine-to-machine | **Not supported** — only users accept shares | **Explicitly supported** |
| Quarantine | Inbound-only by default | Governed by `allowIncomingConnections`; **OPEN** whether quarantine semantics still apply |
| Setup | Invite link, out-of-band, manual accept | Double opt-in policy edits on both tailnets |
| Automatable | Poorly | Fully — policy file is `GET`/`POST /api/v2/tailnet/-/acl` |
| Group sync (SCIM/Google) | n/a | **Unsupported** — can't reference synced groups |

**Consequence:** declarative sharing is the mechanism this design should be built on. Link-based sharing remains the right tool only for the partner/guest case — handing one machine to one outside human.

### Shape of the config

Double opt-in, per the [declarative node sharing docs](https://tailscale.com/docs/features/declarative-node-sharing). Receiving tailnet declares what may be referenced:

```json
{
  "externalTailnets": {
    "prod": {
      "externalID": "<prod-tailnet-id>",
      "allowIncomingConnections": false,
      "allowExternalReferencesTo": ["tag:komodo-controller"]
    }
  }
}
```

Sharing tailnet accepts the relationship and writes the grant:

```json
{
  "externalTailnets": {
    "dev": { "externalID": "<dev-tailnet-id>", "allowIncomingConnections": true }
  },
  "grants": [
    { "src": ["tag://dev/komodo-controller"], "dst": ["tag:host"], "ip": ["8120"] }
  ]
}
```

A grant referencing something absent from the other side's `allowExternalReferencesTo` is **silently ignored** — no error, just no connectivity. That failure mode needs a verification step in any runbook.

## Options

### Option A — Status quo: one tailnet, tag-based separation

- **Pros:** zero migration; flat MagicDNS; one policy file; single console; no alpha dependency.
- **Cons:** isolation is advisory, not structural; one bad grant flattens all environments; no per-environment audit boundary.

### Option B — Three tailnets (Prod / Test / Dev), declarative sharing between them

- **Pros:** isolation is structural — a Dev policy mistake cannot reach Prod; per-tailnet audit logs, auth keys, and tagOwners; Dev becomes genuinely disposable; whole lifecycle is API-drivable (create, `httpsEnabled`, policy push, scoped auth keys); tags and m2m survive the boundary; double opt-in means neither side can unilaterally widen access.
- **Cons:** **depends on an alpha, waitlist-gated feature**; three policy files and three scoped OAuth credentials to manage; FQDNs become tailnet-qualified; silent-ignore failure mode on mismatched references; per-tailnet settings (notably HTTPS certs, off by default) must be provisioned explicitly.

### Option C — Two tailnets: Prod isolated, Test+Dev together

- **Pros:** captures the blast-radius win that matters at two-thirds the overhead; Test↔Dev promotion stays in-tailnet.
- **Cons:** Test loses a hard boundary from Dev, so "does Test mirror Prod?" gets fuzzier; still carries the full alpha dependency for one seam — paying the same risk for less of the benefit.

### Option D — One tailnet, hardened

Policy-file CI with ACL tests in GitHub Actions, mandatory review on the policy file, ban `tag:*` destinations, per-environment Services with their own grants.

- **Pros:** near-zero migration; no alpha dependency; converts soft isolation into *tested* isolation.
- **Cons:** still one flat trust domain; a mistake that passes tests still lands everywhere; audit boundary unchanged.

## Decision

**OPEN.** Recommended starting position for review: **Option D now, Option B when declarative sharing reaches beta/GA.**

Rationale: declarative sharing makes Option B the *architecturally* right answer — it's the first mechanism that makes tailnet-per-environment operationally sane for a single operator, because tags cross, machines can talk, and everything is API-driven. But it is **alpha and waitlist-gated**, and putting the homelab's Prod reachability on an alpha feature inverts the risk this ADR exists to reduce. Option D's policy CI is worth adopting immediately, is useful regardless of the eventual shape, and is a prerequisite for B anyway — three policy files without tests is worse than one policy file without tests.

Option C is not recommended: it takes on the full alpha dependency for a partial boundary.

**Immediate action independent of the decision:** join the declarative node sharing waitlist (admin console → General → Feature previews). Only the *sharing* tailnet needs the feature enabled, and the waitlist is not instant, so the clock should start now.

## Cross-environment flows to inventory before committing

Each of these works today only because everything is one tailnet. Each needs an explicit `externalTailnets`/grant design under B or C:

- Komodo controller (NUC, `tag:admin`) → Prod/Test/Dev periphery agents — the canonical m2m case; workable via `tag://` references, needs port scoping rather than `dst: ["*"]`
- Promotion path DevDocker → Test → Prod (per [ADR-0001](0001-komodo-resourcesync-branch-per-environment.md), promotion is a file move; the network path is currently implicit)
- Observability scrape/push paths across environments — direction matters against `allowIncomingConnections`
- Backup flows to `tag:storage` NAS (UnRaid = Prod, Synology = Test)
- Admin access from the operator's devices to all three — **OPEN:** one identity in three tailnets, or a device per tailnet
- Exit node — one per tailnet, or one shared
- Partner/guest access — keep on **link-based** sharing; don't model humans as external tailnets
- ProxMox host, which hosts the Dev VM but is itself infrastructure

## Trade-offs accepted (under B or C)

- Alpha-feature dependency on the critical path for cross-environment automation.
- Longer FQDNs and a documentation sweep to match.
- Per-tailnet provisioning steps that don't exist today: `httpsEnabled` PATCH, tagOwners, auth keys, policy push.
- Tailnet-scoped OAuth credentials must be captured at creation — the org token cannot administer or delete a tailnet it created.
- Cross-environment automation gets deliberately harder. That's the point, but it's a standing tax.

## Consequences / follow-on work

- Rewrite [tailscale-grants.md](../../network/tailscale-grants.md) per-tailnet. The tag taxonomy shrinks per tailnet — `tag:prod`/`tag:test`/`tag:dev` become redundant once environment identity lives in the boundary.
- Update [tailscale.md](../../network/tailscale.md): Services naming, Docktail behavior, exit node strategy, access tiers.
- Add policy-file CI with ACL tests **now**, single-tailnet or not.
- Decide where policy files live in Git and how they reconcile (API push via CI vs. manual apply) — same question ADR-0001 answered for ResourceSync; the answers should match.
- Reusable provisioning artifact belongs in `artifacts/` per repo convention, not in this doc: OAuth exchange → create tailnet → `httpsEnabled` → tagOwners/policy → scoped auth keys. [`04-api-is-the-way/end-to-end.sh`](https://github.com/frozenprocess/summer-with-tailscale/blob/main/04-api-is-the-way/end-to-end.sh) is a working reference for that exact chain (provisions two tailnets, enables HTTPS, deploys into each, shares them declaratively) and is the obvious starting point to adapt.
- Migration must be reversible with a documented rollback to the single-tailnet state.

## Open questions

1. Plan/cost implications of additional tailnets on the current account.
2. Waitlist timing for declarative node sharing — does it gate the whole plan, and is there a beta ETA?
3. Do quarantine semantics apply to declaratively shared nodes, or does `allowIncomingConnections` fully replace them?
4. Does the operator's own identity/devices need presence in all three tailnets, and how does that interact with the isolation goal?
5. Komodo topology: one controller reaching out via declarative sharing, or a controller per tailnet?
6. Do Services span a tailnet boundary, and does that offer a cleaner contract than node-level grants?
7. Does Test lose meaning as a Prod rehearsal under Option C?
8. Migration order — Dev first (proves the seam cheaply) or Prod first (proves the boundary that matters)?
9. Since synced groups can't be referenced across tailnets, does any current or planned group source conflict with this?

## References

### Primary sources

- [Declarative node sharing](https://tailscale.com/docs/features/declarative-node-sharing) — **the mechanism this ADR is built on.** Alpha, waitlist-gated. `externalTailnets`, double opt-in, `tag://`/`group://` external reference syntax, `allowExternalReferencesTo`, `allowIncomingConnections`. Also the source for the SCIM/Google synced-group limitation.
- [summer-with-tailscale](https://github.com/frozenprocess/summer-with-tailscale) — worked, runnable examples for the API-driven path. Modules used here:
  - [`01-tailnet-sandboxes`](https://github.com/frozenprocess/summer-with-tailscale/blob/main/01-tailnet-sandboxes/readme.md) — OAuth token exchange, `POST /organizations/-/tailnets`, and the tailnet-scoped OAuth credential that the org token cannot substitute for on delete
  - [`02-tailnet-membership`](https://github.com/frozenprocess/summer-with-tailscale/blob/main/02-tailnet-membership/readme.md) — `tsnet` embedded membership; relevant if services join tailnets directly rather than via host daemons
  - [`03-declarative-sharing`](https://github.com/frozenprocess/summer-with-tailscale/blob/main/03-declarative-sharing/readme.md) — `externalTailnets` config walkthrough and scoping external access by group/tag instead of blanket `dst: ["*"]`
  - [`04-api-is-the-way`](https://github.com/frozenprocess/summer-with-tailscale/blob/main/04-api-is-the-way/readme.md) — policy file as data via `GET`/`POST /tailnet/-/acl`, the `httpsEnabled` PATCH, scoped/tagged/short-lived auth keys, plus `end-to-end.sh`

### Supporting

- [Share your machines with other users](https://tailscale.com/docs/features/sharing) — link-based sharing. Source of the tag-stripping, quarantine, and user-scoped constraints; retained here for the partner/guest case and to keep the two mechanisms distinguishable.
- [Tailscale Services GA](https://tailscale.com/blog/services-ga) — declarative JSON config, per-service audit logs, ACL tests, remote-destination proxies
- [Grants syntax](https://tailscale.com/docs/reference/syntax/grants)
- [Tailnet policy file](https://tailscale.com/docs/features/tailnet-policy-file)
- [Tailscale API reference](https://tailscale.com/api)
