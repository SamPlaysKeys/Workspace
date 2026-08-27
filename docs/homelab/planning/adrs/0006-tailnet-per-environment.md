---
type: ADR
Status: Rejected
Date: 2026-08-26
agent_generated: true
agent_at: "2026-08-26T00:00:00Z"
review_status: "awaiting_review"
reviewed_by: ""
last_modified_by: "agent"
---

# ADR: Tailnet-per-environment (Prod / Test / Dev) with declarative node sharing

## Status

**Rejected.** The homelab will **not** split Prod/Test/Dev into separate tailnets. Environment separation stays as it is today: a single tailnet with tag-based grants.

The investigation is retained in full because the rejection is the useful part — it documents why an appealing structural-isolation story doesn't survive contact with an already-working Services deployment.

Superseded in direction by **[ADR-0007: Workload tailnet separation](0007-workload-tailnet-separation.md)**, which applies the tailnet boundary where it actually pays for itself: separating non-homelab workload from the homelab, rather than separating homelab environments from each other.

## Context

Today the homelab runs a **single tailnet** with tag-based environment separation. Services are already the primary exposure mechanism for containers across Prod/Test/Dev — advertised via Docktail labels and ScaleTail sidecars, with per-service MagicDNS names, per [tailscale.md](../../network/tailscale.md). That part of the design is settled and this ADR does not revisit it.

What is *not* settled is the environment boundary. Per [tailscale-grants.md](../../network/tailscale-grants.md), isolation between Prod/Test/Dev is enforced entirely by grant rules:

```json
{"src": ["tag:prod"], "dst": ["tag:prod"], "ip": ["*:*"]}
{"src": ["tag:test"], "dst": ["tag:test"], "ip": ["*:*"]}
{"src": ["tag:dev"], "dst": ["tag:dev"], "ip": ["*:*"]}
```

That works, but the isolation is **one policy-file edit deep**. A bad merge, a mis-tagged node, or a fat-fingered `tag:*` destination collapses Prod/Test/Dev into one flat network. The blast radius of a policy mistake is the entire homelab.

Two Tailscale capabilities change the calculus:

1. **Additional tailnets via the API.** `POST /api/v2/organizations/-/tailnets` provisions a fully isolated tailnet under the same account, no console clicks. The response carries a **tailnet-scoped OAuth client** — creation is org-scoped, but administration and deletion require that per-tailnet credential ([worked example](https://github.com/frozenprocess/summer-with-tailscale/blob/main/01-tailnet-sandboxes/readme.md)).
2. **Declarative node sharing** ([docs](https://tailscale.com/docs/features/declarative-node-sharing), **alpha, waitlist-gated**). Cross-tailnet access becomes a policy-file construct — `externalTailnets` plus grants referencing `group://<alias>/<name>` or `tag://<alias>/<name>`. It is explicitly designed for **machine-to-machine access between trusted tailnets**, which is exactly the homelab's shape.

The question this ADR frames: **is a tailnet boundary the right isolation primitive for the homelab's environments, and is the cross-tailnet seam cheap enough to live with?**

### What a tailnet split does to the existing Services layer

Because Services are already the exposure model, the split's real cost lands here rather than on greenfield design:

- **Service names become tailnet-qualified.** Every service currently resolves under one tailnet's MagicDNS suffix. Three tailnets means three suffixes, and the naming convention documented in [tailscale.md](../../network/tailscale.md) needs an environment-aware revision. **OPEN:** does `plex.<prod-tailnet>.ts.net` replace the current flat convention, or does an alias layer hide the split?
- **`httpsEnabled` is per-tailnet and off by default.** Any new tailnet needs the setting turned on before TLS-terminating services work at all — a provisioning step that doesn't exist today because the current tailnet was configured once, long ago.
- **Docktail/ScaleTail config becomes tailnet-scoped.** Sidecars and label-driven advertisement authenticate to a specific tailnet. Auth keys, tags, and tagOwners all become per-tailnet concerns.
- **Cross-environment service consumption is the seam.** Anything in one environment consuming a service in another (observability, backups, promotion tooling) stops being an in-tailnet lookup and becomes an `externalTailnets` grant.

**OPEN and worth answering early:** whether a Service can be *published across* a tailnet boundary. If yes, the boundary contract is service-level and clean. If not, cross-environment access is node-and-port level, which is coarser than the exposure model already in use — a genuine regression against current practice, and arguably the strongest argument for staying on one tailnet.

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
| Churn to the existing Services layer | Medium | DNS suffixes, sidecar auth, per-tailnet `httpsEnabled` |
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

Retained as the investigation record. Outcome noted on each.

### Option A — Status quo: one tailnet, tag-based separation — **CHOSEN**

- **Pros:** zero migration; flat MagicDNS; one policy file; single console; no alpha dependency; Services and access model untouched.
- **Cons:** isolation is advisory, not structural; one bad grant flattens all environments; no per-environment audit boundary.

### Option B — Three tailnets (Prod / Test / Dev), declarative sharing between them — rejected

- **Pros:** isolation is structural — a Dev policy mistake cannot reach Prod; per-tailnet audit logs, auth keys, and tagOwners; Dev becomes genuinely disposable; whole lifecycle is API-drivable (create, `httpsEnabled`, policy push, scoped auth keys); tags and m2m survive the boundary; double opt-in means neither side can unilaterally widen access.
- **Cons:** **depends on an alpha, waitlist-gated feature**; three policy files and three scoped OAuth credentials to manage; FQDNs become tailnet-qualified; silent-ignore failure mode on mismatched references; per-tailnet settings (notably HTTPS certs, off by default) must be provisioned explicitly; rewrites a working Services exposure surface.

### Option C — Two tailnets: Prod isolated, Test+Dev together — rejected

- **Pros:** captures the blast-radius win that matters at two-thirds the overhead; Test↔Dev promotion stays in-tailnet.
- **Cons:** Test loses a hard boundary from Dev, so "does Test mirror Prod?" gets fuzzier; still carries the full alpha dependency for one seam — paying the same risk for less of the benefit.

### Option D — One tailnet, hardened — **CARRIED FORWARD** (with A)

Policy-file CI with ACL tests in GitHub Actions, mandatory review on the policy file, ban `tag:*` destinations, and tighten per-service grants on the Services already in place.

- **Pros:** near-zero migration; no alpha dependency; converts soft isolation into *tested* isolation.
- **Cons:** still one flat trust domain; a mistake that passes tests still lands everywhere; audit boundary unchanged.

## Decision

**Rejected — keep the current single-tailnet structure with tag-based environment separation.**

The structural-isolation argument is real, and declarative node sharing does make tailnet-per-environment *technically* achievable in a way link-based sharing never did. It is still the wrong trade for this homelab. Four reasons, in order of weight:

### 1. It breaks access that currently works, to solve a problem that hasn't bitten

Services are already the exposure mechanism for most containers, and access works today — admin tiers, partner access, per-service MagicDNS names. A tailnet split rewrites that working surface: tailnet-qualified DNS suffixes, tailnet-scoped Docktail/ScaleTail sidecar auth, per-tailnet `httpsEnabled`, and per-tailnet auth keys and tagOwners. The failure it prevents — a fat-fingered grant flattening environments — has not actually occurred. **Trading working access for hypothetical blast-radius reduction is the wrong direction.**

### 2. Cross-boundary sharing is coarser than what's already in place

The unresolved question from the investigation is decisive: if a Service cannot be published across a tailnet boundary, then cross-environment access degrades to node-and-port grants. That is **coarser than the per-service exposure model already running**. And if the shareable app has to physically live in the tailnet it's shared from, containers move off the Prod Docker host — which drags in Komodo stack placement and the promotion path from [ADR-0001](0001-komodo-resourcesync-branch-per-environment.md). Either answer is worse than the status quo for a boundary nobody asked for.

### 3. The complexity is not proportionate to a single-operator homelab

Three tailnets means three policy files, three consoles, three sets of tailnet-scoped OAuth credentials, three auth-key rotations, and per-tailnet provisioning steps that don't exist today. Add the **silent-ignore failure mode** — a grant referencing something absent from the other side's `allowExternalReferencesTo` produces no error, just no connectivity — and routine debugging gets materially harder. Isolation that is expensive to operate gets worked around, and a worked-around boundary is worse than an honest tag.

### 4. The alpha dependency lands on the wrong path

Declarative node sharing is alpha and waitlist-gated. Under this ADR it would sit on the path between the Komodo controller and Prod periphery agents — meaning homelab *manageability* depends on an alpha feature. That inverts the risk this ADR set out to reduce.

### What is kept

- **Environment separation stays tag-based** (`tag:prod` / `tag:test` / `tag:dev`) in one tailnet. No grants rewrite, no tag taxonomy change, no Services churn.
- **Partner access stays link-based sharing**, per the adversarial model in [tailscale.md](../../network/tailscale.md). Outside humans are not modeled as external tailnets.
- **Policy-file CI with ACL tests is still worth doing** (the former Option D) and is the only recommendation carried forward from this ADR. It converts advisory isolation into *tested* isolation at near-zero cost, and it is independently useful.

### Future option: collapse three environments to two

Noted, not decided: the Prod/Test/Dev split could later collapse to **Test and Prod only**. Under [ADR-0007](0007-workload-tailnet-separation.md), Dev's remaining purpose is being reconsidered — if non-homelab work moves to a workload environment, a third homelab environment may not be earning its keep. That is a separate decision and deliberately out of scope here.

### Where the tailnet boundary does belong

The investigation's real finding is that the boundary axis was wrong. Splitting *environments* imposes a boundary across paths that need to stay open (admin, Komodo, observability, backups). Splitting *workload from homelab* puts the boundary where the paths should be closed anyway. See **[ADR-0007](0007-workload-tailnet-separation.md)**.

## Cross-environment flows that made the split expensive

Each of these works today only because everything is one tailnet. Each would have needed an explicit `externalTailnets`/grant design — collectively, this list is the concrete form of reason #1 above:

- Komodo controller (NUC, `tag:admin`) → Prod/Test/Dev periphery agents — the canonical m2m case; technically workable via `tag://` references, but it puts homelab manageability behind an alpha feature
- Promotion path DevDocker → Test → Prod (per [ADR-0001](0001-komodo-resourcesync-branch-per-environment.md), promotion is a file move; the network path is currently implicit and would have to become explicit)
- Observability scrape/push paths across environments — direction matters against `allowIncomingConnections`
- Backup flows to `tag:storage` NAS (UnRaid = Prod, Synology = Test)
- Admin access from the operator's devices to all three — one identity in three tailnets, or a device per tailnet, neither cheap
- Exit node — one per tailnet, or one shared
- Partner/guest access — stays on link-based sharing regardless
- ProxMox host, which hosts the Dev VM but is itself infrastructure

## Trade-offs that would have been accepted (and were judged too costly)

- Alpha-feature dependency on the critical path for cross-environment automation.
- Longer FQDNs and a documentation sweep to match.
- Per-tailnet provisioning steps that don't exist today: `httpsEnabled` PATCH, tagOwners, auth keys, policy push.
- Tailnet-scoped OAuth credentials must be captured at creation — the org token cannot administer or delete a tailnet it created.
- Cross-environment automation gets deliberately harder. Intended for environments; unhelpful for admin paths.

## Consequences / follow-on work

Because the split is rejected, most of the original follow-on work is moot. What remains:

- **Add policy-file CI with ACL tests.** The one carried-forward recommendation. Ban `tag:*` destinations, require review on the policy file, run tests in GitHub Actions.
- **No changes** to [tailscale-grants.md](../../network/tailscale-grants.md) or [tailscale.md](../../network/tailscale.md) from this ADR — the tag taxonomy and Services naming convention stand as documented.
- **Record the rejection** so the option isn't re-litigated from scratch; the mechanism comparison above is the reusable part.
- Continue in [ADR-0007](0007-workload-tailnet-separation.md) for the workload boundary, where the provisioning artifact and waitlist question actually apply.

## Open questions — resolved or relocated

The rejection closes most of these. Recorded so the option isn't re-litigated from scratch:

1. Plan/cost implications of additional tailnets — **moot here**; relocated to [ADR-0007](0007-workload-tailnet-separation.md).
2. Waitlist timing for declarative node sharing — **no longer gating** this decision; still worth joining for ADR-0007.
3. Quarantine semantics vs. `allowIncomingConnections` for declaratively shared nodes — **unresolved**; relocated to ADR-0007, where it actually matters.
4. Operator identity/devices across three tailnets — **moot**; admin access stays in one tailnet, which is the point of the rejection.
5. Komodo topology across a boundary — **moot**; controller and periphery agents stay in one tailnet.
6. Can a Service be published across a tailnet boundary? — **unresolved and now load-bearing for [ADR-0007](0007-workload-tailnet-separation.md).** Answering it decides whether shareable apps can stay on their current host or must physically move.
7. Does Test lose meaning as a Prod rehearsal under Option C? — **moot**; Option C rejected. Related but distinct question of collapsing to Test+Prod is noted as a future option above.
8. Migration order — **moot**; no migration.
9. Synced groups (SCIM/Google) can't be referenced across tailnets — **relocated** to ADR-0007; only relevant where a boundary exists.

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
- [Tailscale Services GA](https://tailscale.com/blog/services-ga) — cited for the GA-era additions relevant to a split (declarative on-node JSON config, per-service audit logs, ACL tests), not as an introduction to Services, which the homelab already runs
- [Grants syntax](https://tailscale.com/docs/reference/syntax/grants)
- [Tailnet policy file](https://tailscale.com/docs/features/tailnet-policy-file)
- [Tailscale API reference](https://tailscale.com/api)
