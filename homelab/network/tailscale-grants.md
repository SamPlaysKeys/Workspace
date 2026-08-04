---
type: Reference
status: Not Reviewed
reference: '[[tailscale]]'
layout: page
title: Tailscale Grants
category: Homelab
---
{% raw %}


# Tailscale Grants

Access control policy for the homelab tailnet. Defines tags, groups, and grants mapping the access tier model to Tailscale policy.

Uses Tailscale Grants syntax — the modern replacement for ACLs. Grants remove the redundant `action` field, separate destinations from ports, and support application-layer capabilities.

---

## Tag Inventory

Seven tags organize the tailnet. Every device gets one or more tags. `tag:host` and `tag:storage` are informational only — applied for sorting/filtering in the admin console, not referenced in any grant rules.

| Tag | Applied To | Purpose | Assigned By |
|-----|------------|---------|-------------|
| `tag:admin` | Admin interfaces (Komodo UI, ProxMox, router) | Infrastructure management UI | `autogroup:admin` |
| `tag:host` | All Docker hosts, VMs, physical machines | Base machine access (SSH, agent comms) | `autogroup:admin` |
| `tag:storage` | NAS devices (UnRaid, Synology) | File shares (NFS, SMB) | `autogroup:admin` |
| `tag:prod` | Production nodes | Production environment | `autogroup:admin` |
| `tag:test` | Test nodes | Test environment | `autogroup:admin` |
| `tag:dev` | Development nodes | Dev/sandbox environment | `autogroup:admin` |
| `tag:app` | Nodes advertising user-facing services | Application UIs exposed via Docktail | `autogroup:admin` |

### Composite Tags

Most devices carry multiple tags. Examples:

- **LenovoMini 1 (Prod Docker host):** `tag:host,tag:prod,tag:app`
- **LenovoMini 2 (Test Docker host):** `tag:host,tag:test,tag:app`
- **ProxMox host:** `tag:host,tag:dev`
- **UnRaid NAS:** `tag:storage,tag:prod`
- **Synology NAS:** `tag:storage,tag:test`
- **NUC (Komodo Controller):** `tag:host,tag:admin`
- **Exit node:** `tag:host` only (no services, no storage, no app exposure)

### Tag Ownership Reasoning

`tag:app` is owned by `autogroup:admin`. New services go through admin approval.

---

## User Groups

Two groups map to the access tiers from the Tailscale strategy doc.

| Group | Members | Access Scope |
|-------|---------|--------------|
| `group:admin` | Owner | Everything — infra, apps, storage, SSH, subnets |
| `group:user` | Partner | User-facing app UIs only |

```json
{
  "groups": {
    "group:admin": ["you@domain.com"],
    "group:user": ["partner@domain.com"]
  }
}
```

---

## Grant Rules

Rules are grouped by access pattern. Each group shows the principle, then the grant.

Grants omit `action` (implied accept), separate destination (`dst`) from ports/protocols (`ip`).

### 1. Admin — Full Access

Admins reach every tagged device on every port. This is the safety valve — no rule below can block admin access.

```json
{"src": ["group:admin"], "dst": ["tag:*"], "ip": ["*:*"]}
```

### 2. User — Web App Access

Users reach user-facing application UIs only. Web ports only (80, 443).

```json
{"src": ["group:user"], "dst": ["tag:app"], "ip": ["80", "443"]}
```

This gives partner access to:
- `actual.tailnet.ts.net` → Actual Budget
- `plex.tailnet.ts.net` → Plex
- Any other service on a `tag:app` node

It does **not** give access to:
- App admin panels on non-standard ports
- Infrastructure UIs (Komodo, ProxMox)
- SSH

### 3. SSH Access

Admins can SSH into any tagged node. Covered by rule 1 (port 22 within `*:*`). Tailscale SSH handles key management — no separate SSH key infrastructure needed.

### 4. Environment Isolation

Prod, test, and dev environments cannot reach each other by default. Admins bypass this via rule 1.

```json
{"src": ["tag:prod"], "dst": ["tag:prod"], "ip": ["*:*"]}
{"src": ["tag:test"], "dst": ["tag:test"], "ip": ["*:*"]}
{"src": ["tag:dev"], "dst": ["tag:dev"], "ip": ["*:*"]}
```

Cross-environment traffic must be explicitly added (e.g., test→prod DB replication) or use admin access.

### 5. App-to-Admin (Optional)

If apps need to reach admin interfaces (e.g., Komodo agent registration), uncomment:

```json
{"src": ["tag:app"], "dst": ["tag:admin"], "ip": ["443"]}
```

### 6. Subnet Routes (Admin-Only)

Subnet routes exist for admin management of non-Tailscale devices (switches, APs, IoT). Only admins can reach them.

```json
{"src": ["group:admin"], "dst": ["10.0.50.0/24"], "ip": ["*:*"]}
```

Regular users are denied by default — no explicit rule needed since Tailscale denies by default.

---

## Complete Policy

```json
{
  "groups": {
    "group:admin": ["you@domain.com"],
    "group:user": ["partner@domain.com"]
  },
  "tagOwners": {
    "tag:admin": ["autogroup:admin"],
    "tag:host": ["autogroup:admin"],
    "tag:storage": ["autogroup:admin"],
    "tag:prod": ["autogroup:admin"],
    "tag:test": ["autogroup:admin"],
    "tag:dev": ["autogroup:admin"],
    "tag:app": ["autogroup:admin"]
  },
  "grants": [
    {"src": ["group:admin"], "dst": ["tag:*"], "ip": ["*:*"]},
    {"src": ["group:user"], "dst": ["tag:app"], "ip": ["80", "443"]},
    {"src": ["tag:prod"], "dst": ["tag:prod"], "ip": ["*:*"]},
    {"src": ["tag:test"], "dst": ["tag:test"], "ip": ["*:*"]},
    {"src": ["tag:dev"], "dst": ["tag:dev"], "ip": ["*:*"]},
    {"src": ["group:admin"], "dst": ["10.0.50.0/24"], "ip": ["*:*"]}
  ]
}
```

---

## Tests

Tailscale policy tests validate rules before deployment. These should be added to the Admin Console alongside the policy.

```json
{
  "tests": [
    {
      "src": "you@domain.com",
      "accept": ["tag:admin:443", "tag:app:443", "10.0.50.1:443"],
      "deny": []
    },
    {
      "src": "partner@domain.com",
      "accept": ["tag:app:443"],
      "deny": ["tag:admin:443"]
    }
  ]
}
```

---

## SSH Rules

Tailscale SSH is enabled for admin access to any tagged node.

```json
{
  "ssh": [
    {
      "action": "check",
      "src": ["group:admin"],
      "dst": ["tag:*"],
      "users": ["root", "ubuntu", "sam"]
    }
  ]
}
```

No SSH access for users.

---

## Default Deny

Tailscale Grants are deny-by-default. Any access not explicitly allowed above is blocked.

Implicitly denied:
- Users accessing infra UIs (Komodo, ProxMox)
- Users SSH
- Cross-environment traffic (except via admin)
- All traffic from untagged devices

---

## Auto-Approvers

Required for Docktail to automatically advertise services without manual approval.

```json
{
  "autoApprovers": {
    "services": {
      "tag:app": ["autogroup:admin"]
    }
  }
}
```

Admins can approve Tailscale Services tagged with `tag:app`.

---

## Tag Assignment Examples

### Docker Host in Production (LenovoMini 1)

```bash
sudo tailscale up \
  --advertise-tags=tag:host,tag:prod,tag:app \
  --ssh
```

### NAS (UnRaid)

```bash
sudo tailscale up \
  --advertise-tags=tag:storage,tag:prod
```

### Komodo Controller (NUC)

```bash
sudo tailscale up \
  --advertise-tags=tag:host,tag:admin \
  --ssh
```

### Exit Node

```bash
sudo tailscale up \
  --advertise-tags=tag:host \
  --advertise-exit-node
```

---

## Open Questions

- [x] Storage port range — resolved: `*:*`
- [x] Cross-environment sync — resolved: none needed
- [x] `group:manager` — resolved: not used, removed
- [x] Exit node grants — resolved: dedicated node handles this
- [ ] IoT subnet (`10.0.50.0/24`) — likely will change, placeholder

{% endraw %}