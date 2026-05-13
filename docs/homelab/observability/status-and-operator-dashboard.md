# Public status site and operator dashboard

Plans for **user-facing** public status (`status.samplayskeys.com`) and a **private** operator “single pane of glass,” with acceptance criteria, sequencing, and open decisions.

---

## Phase 1 — Public status site

**URL:** `status.samplayskeys.com`  
**Audience:** Partner and other **users** of user-facing services (not homelab admins).  
**Tone / UX:** Similar to [GitHub Status](https://www.githubstatus.com/): components (or service groups), overall status, recent incident history, minimal jargon.

### Intent

- **User-available observability:** clear “is the thing I use up?” without exposing internal topology or admin tools.
- **Survives local outage:** host on a **VPS** (candidate: Oracle Cloud free tier) so the page stays reachable if home internet or the lab is down.

### Success criteria

1. Public HTTPS at `status.samplayskeys.com` with a sensible TLS story (e.g. reverse proxy + Let’s Encrypt on the VPS).
2. Lists **only non-admin, user-relevant** resources (e.g. Plex, Signal, Miniflux, Syncthing client-facing checks — exact list is yours to curate).
3. Does **not** link to or name internal admin URLs, VLANs, or management UIs.
4. Status data remains trustworthy when the house is offline: checks must run from the **VPS** (or another off-LAN path) against **reachable** endpoints (public hostname, reverse proxy, or approved tunnel — not `10.x.x.x`).
5. Optional but valuable: brief incident log / maintenance notices you can edit without redeploying the whole stack (even a static markdown or small API is fine).

### Open decisions (Phase 1)

| Topic | Notes |
|--------|--------|
| **Product / implementation** | Options include self-hosted status engines (e.g. Cachet, Statping-ng, OpenStatus-style), static site + scheduled probe publishing JSON, or minimal custom static + GitHub-style updates. Pick based on maintenance appetite and whether you want a DB on the VPS. |
| **Source of truth vs Uptime Kuma** | Kuma may stay **authoritative for alerting** on the LAN; the public site can **duplicate** checks from the VPS or **subscribe** to exported state if you add a safe integration. Avoid coupling public page to LAN-only Kuma without a tunnel or push path. |
| **DNS** | `status` subdomain DNS at your registrar → VPS public IP; confirm Oracle (or chosen host) outbound checks allowed to your public endpoints. |

### Non-goals (Phase 1)

- Full metrics graphs, Prometheus, or environmental sensors (already deferred elsewhere).
- Authenticated “admin view” of the same page (keep public surface boring and safe).

---

## Phase 2 — Single pane of glass (operator)

**Audience:** You (operator), not the general public.  
**Primary job:** One place to **orient** and **jump** to deeper tools.

### Intent

1. **Overview of the entire lab** — health/signal at a glance (CPU/load/disk on key hosts, maybe Docker summary), possibly via **Glances** or equivalent on hosts plus an aggregation story.
2. **Curated links** (always visible):
   - **Uptime Kuma** (alerting / synthetics).
   - **Komodo** (deployments / stacks).
3. **Dynamic or data-driven links** to **management** URLs: Unraid UI, container admin UIs, UniFi, Synology DSM, etc. — ideally driven from the same source of truth you use for Komodo or a small YAML manifest so new services do not require hand-editing HTML.

### Success criteria

1. One bookmarkable URL (likely **not** public — Tailscale or LAN-only) that opens the dashboard.
2. Sections or tiles: “Lab overview” (metrics snapshot), “Monitoring”, “Orchestration”, “Management consoles” (grouped by host or role).
3. Every critical jump target reachable from that page in ≤2 clicks without remembering IPs.
4. Link list can be updated without fragile one-off edits (manifest, template, or generator).

### Open decisions (Phase 2)

| Topic | Notes |
|--------|--------|
| **Glances vs hybrid** | Glances is strong **per-host**; “whole lab” may mean Glances on LenovoMinis + NUC + ProxMox with a **portal** (Homer, Dashy, Heimdall, Homarr) that embeds or links. Pure Glances multi-server exists but compare ops cost vs a small static portal + Glances where you care most. |
| **Hosting** | Typically homelab-side (or Tailscale-only) — **not** the same trust boundary as the public status site. |
| **Secrets in URLs** | Management UIs often should not be in public git; prefer env-backed or encrypted manifest if URLs contain tokens (usually they should not — use SSO or Tailscale Serve). |

### Relationship to Phase 1

| | Phase 1 (public status) | Phase 2 (operator dashboard) |
|--|-------------------------|-------------------------------|
| Host | VPS | Homelab / Tailscale |
| Audience | Users | You |
| Data | Synthetic / public checks | Richer metrics + deep links |
| Alerting | Informational; Kuma can still alert you | Kuma primary; dashboard is navigation |

---

## Suggested sequencing

1. **Inventory** user-facing hostnames and which are probe-able from the public internet (required for Phase 1).
2. Stand up **VPS + DNS + TLS** for `status.samplayskeys.com`.
3. Implement **Phase 1** status page and curate components.
4. **Phase 2:** choose portal + Glances (or alternative) layout, add manifest for management links, wire Komodo/Kuma bookmarks.

---

## Pre-mortem hooks (when you pick tools)

- **Status on VPS:** “Checks pass from Oracle but users still can’t reach the app” — split **your** connectivity vs **user** path (DNS, CDN, home IP change).
- **Operator dashboard:** “I pasted every admin URL into git” — rotation and leak risk; prefer Tailscale MagicDNS names + no secrets in repo.

When you name concrete software for Phase 1 or 2, run a short **Pre-Mortem** before locking in.
