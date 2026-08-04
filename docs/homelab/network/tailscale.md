---
type: Reference
subcategory: Network
category: Homelab
status: Active
date: 2026-08-03
---

# Tailscale

Network connectivity strategy for the homelab using Tailscale mesh VPN.

---

## Overview

Tailscale is a mesh VPN built on WireGuard that creates a secure network (called a "tailnet") across devices regardless of their physical location. Unlike traditional VPNs, there's no central server that all traffic flows through — devices connect directly to each other.

This homelab leverages **Tailscale Services** as the primary mechanism for exposing applications, with ACL-based access control separating user-facing interfaces from administrative access.

### Key Concepts

- **Tailnet** — Your private network of connected devices
- **MagicDNS** — Automatic DNS for devices (e.g., `device-name.tailnet-name.ts.net`)
- **Tailscale Services** — Named services advertised to the tailnet, accessible via MagicDNS
- **ACLs** — Access control lists defining who can reach what
- **Funnel** — Expose services to the public internet through Tailscale
- **Serve** — Expose local services to your tailnet

---

## Advantages Over Traditional VPNs

| Traditional VPN | Tailscale |
|-----------------|-----------|
| Hub-and-spoke — all traffic through central server | Mesh — direct device-to-device connections |
| Central server is bottleneck and single point of failure | No central infrastructure to fail |
| Complex setup (certificates, port forwarding, firewall rules) | Zero-config in most cases |
| Requires exposing a port to the internet | NAT traversal built-in, no exposed ports |
| VPN server needs to be always on | Coordination server only for handshake, not traffic |
| All-or-nothing access | Granular ACLs per user/device/service |

**Why this matters for homelab:**
- No need to open ports on home router
- Access homelab from anywhere without complex setup
- Granular control over who accesses what
- Works even when home IP changes

---

## Tailscale Services

**Tailscale Services** are the primary mechanism for exposing applications in this homelab. Rather than exposing entire hosts or subnets, individual services are advertised to the tailnet with their own DNS names.

### Why Services Over Hosts

| Approach | Exposure | Granularity |
|----------|----------|-------------|
| Host-based | Entire machine accessible | All ports, all services |
| Subnet routing | Entire network accessible | Everything on the subnet |
| **Services** | Single application endpoint | One port, one purpose |

Services provide the tightest blast radius — if a service is compromised, the attacker gains access to that service only, not the host or network.

### Service Naming Convention

Services are accessible via MagicDNS:
```
<service-name>.tailnet-name.ts.net
```

Examples:
- `actual.tailnet.ts.net` — Actual Budget
- `plex.tailnet.ts.net` — Plex Media Server
- `komodo.tailnet.ts.net` — Komodo UI (admin-only)

---

## Container Exposure Patterns

Three patterns for connecting containerized services to Tailscale:

### Docktail — Service Advertisement

[Docktail](https://github.com/marvinvr/docktail) advertises containers as **Tailscale Services**. Use this when:
- The application needs to be reachable by name from the tailnet
- Multiple users/devices will access the service
- You want the service in MagicDNS

Docktail watches for Docker labels and advertises matching containers as services.

### ScaleTail — Per-Service Sidecar Exposure

[ScaleTail](https://github.com/2Tiny2Scale/ScaleTail) provides Docker Compose configurations using the **sidecar pattern** — each service gets its own Tailscale container via `network_mode: service:`. The sidecar can advertise the service to the tailnet with Tailscale Serve.

Use this when:
- You want per-service Tailscale instances (not centralized)
- The container needs both inbound and outbound tailnet access
- You want explicit control per service

### Outbound-Only Sidecar

A sidecar container that joins the tailnet but **does not advertise a service**. Use this when:
- The container needs to reach other tailnet resources
- The container should not be directly addressable
- You need tailnet access for configuration pulls, secrets, etc.

Example: A container that pulls secrets from Vault over Tailscale but doesn't need inbound connections.

### Choosing Between Them

| Need | Pattern |
|------|---------|
| Users access this app, want centralized | Docktail (labels-based) |
| Per-service control, inbound + outbound | ScaleTail (sidecar pattern) |
| App needs tailnet access, no inbound | Outbound-Only Sidecar |
| Both inbound and outbound (centralized) | Docktail (service already has tailnet access) |

---

## High Availability with Services

Tailscale Services can be advertised from **multiple machines** for high availability. When the same service name is advertised from multiple nodes, Tailscale routes traffic to an available instance.

### Multi-Node Service Advertisement

```
┌─────────────────┐     ┌─────────────────┐
│   Prod-Node-1   │     │   Prod-Node-2   │
│                 │     │                 │
│  ┌───────────┐  │     │  ┌───────────┐  │
│  │  Docktail │  │     │  │  Docktail │  │
│  │           │  │     │  │           │  │
│  │ advertises│  │     │  │ advertises│  │
│  │ "plex"    │  │     │  │ "plex"    │  │
│  └─────┬─────┘  │     │  └─────┬─────┘  │
└────────┼────────┘     └────────┼────────┘
         │                       │
         ▼                       ▼
    ┌─────────────────────────────────┐
    │  plex.tailnet.ts.net            │
    │  (routes to available node)     │
    └─────────────────────────────────┘
```

### HA Candidates

Services that benefit from multi-node advertisement:
- Reverse proxies / ingress
- Stateless APIs
- Load-balanced workers

Services that should remain single-node:
- Stateful databases (unless using clustering)
- Apps with local storage dependencies

---

## Access Tiers

Not all users should access all interfaces. This homelab distinguishes between **user-facing UIs** and **admin portals**.

### Role Definitions

| Role | Description | Example Users |
|------|-------------|---------------|
| **user** | Can access consumer-facing application UIs | Partner, guests |
| **manager** | Can access app admin panels, not infrastructure | — |
| **admin** | Full access including infrastructure management | Owner |

### Interface Separation

| Interface Type | Accessible To | Examples |
|----------------|---------------|----------|
| User Web UI | All tailnet users | Plex, Actual Budget, Jellyfin |
| App Admin Portal | managers, admins | Plex admin, *arr settings |
| Infrastructure UI | admins only | Komodo, Proxmox, router admin |
| SSH/Console | admins only | All hosts |

### Implementation Approach

ACLs enforce access tiers by:
1. Grouping users into roles (groups in ACL policy)
2. Tagging services by interface type
3. Granting access based on role → tag mappings

See [[tailscale-grants]] for the full grants policy document — this section defines the model, not the implementation.

---

## Subnet Access Strategy

Subnet routing exposes entire network ranges to the tailnet. This is powerful but dangerous — it bypasses per-service access control.

### When to Use Subnets

Subnet routing is appropriate for:
- **Admin management** of devices that can't run Tailscale (switches, APs, IoT devices)
- **Legacy systems** that predate the Tailscale deployment
- **Temporary access** during migrations

### Admin-Only Subnet Access

Subnet routes are advertised but **restricted to admins only** via ACLs. Regular users and managers cannot reach devices via subnet routing — they must use advertised services.

```
Admin ──(subnet route)──▶ 10.0.50.0/24 (IoT VLAN) ──▶ Switch management UI
User  ──(blocked)──────▶ 10.0.50.0/24
```

This preserves the principle of least privilege: users access applications, admins access infrastructure.

### Subnets vs Services

| Access Method | User | Manager | Admin |
|---------------|------|---------|-------|
| Tailscale Services | Per-ACL | Per-ACL | Yes |
| Subnet Routes | No | No | Yes |

---

## Exit Node Strategy

Exit nodes route all device traffic through a tailnet node. Useful for:
- Appearing to be "at home" when traveling
- Using home DNS/Pi-hole from anywhere
- Accessing geo-restricted content

### The Exposure Risk

An exit node that also advertises subnet routes or services creates a risk: a device using the exit node could potentially reach resources it shouldn't.

### Mitigation: Dedicated Exit Node

Run exit node functionality on a **dedicated node** that:
- Advertises itself as an exit node
- Does **not** advertise subnet routes
- Does **not** run Docktail or advertise services
- Has minimal local services

This isolates the exit node function from service exposure. Traffic exits to the internet through the node, but doesn't gain access to internal resources.

```
┌─────────────────────────────────┐
│  Exit Node (dedicated)          │
│                                 │
│  ✓ Exit node enabled            │
│  ✗ No subnet routes             │
│  ✗ No Docktail                  │
│  ✗ No local services            │
│                                 │
│  Internet ◀── traffic exits     │
└─────────────────────────────────┘
```

### Alternative: ACL-Controlled Exit

If a dedicated node isn't practical, ACLs can restrict what exit node users can reach. But this is more complex and easier to misconfigure — the dedicated node approach is preferred.

---

## Use Cases

### Remote Application Access

Applications are exposed as Tailscale Services via Docktail:

```
Phone/Laptop ──(Tailscale)──▶ actual.tailnet.ts.net ──▶ Actual Budget container
```

- No public exposure
- Accessible from any device on the tailnet
- Access controlled by ACLs based on user role

### Remote Device Access

**Tailscale SSH:**
- SSH into any device on the tailnet without managing SSH keys
- Tailscale handles authentication via identity provider
- Restricted to admins via ACLs
- `ssh user@device.tailnet.ts.net`

**Management UIs (admin-only):**
- Proxmox, Komodo, router admin panels
- Accessible only to admin role
- Either via advertised service or subnet route

### Remote File Access

**SMB shares:**
- Access Windows/Samba shares from anywhere
- `smb://nas.tailnet.ts.net/share`
- ACLs control which shares are accessible to which roles

**NFS mounts:**
- Mount NFS shares over Tailscale
- Performance may degrade over high-latency connections

---

## Shared Access (Partner)

Partner has her own Tailscale account and devices, shared into the tailnet with controlled access. She is a member of the **user** role.

### Adversarial Access Model

Treat partner's access as **adversarial by default** — not because of distrust, but because:
- Her devices may be compromised
- Mistakes happen (accidental deletion, misconfiguration)
- Principle of least privilege — only grant what's needed

### Partner Access (User Role)

| Resource | Access | Rationale |
|----------|--------|-----------|
| User Web UIs (Plex, Actual, etc.) | Yes | Shared apps |
| App Admin Portals | No | No management need |
| Infrastructure UIs | No | No management need |
| SSH/Console | No | No direct machine access |
| Subnet Routes | No | Admin-only |
| File shares | Limited folders | Only shared content |

### Environment-Based Protections

| Environment | Partner Access |
|-------------|----------------|
| Prod | User Web UIs only |
| Test | No |
| Dev | No |

Prod services exposed to partner are explicitly whitelisted as user-tier services. Everything else is deny-by-default.

---

## Docktail Implementation

[Docktail](https://github.com/marvinvr/docktail) runs as an independent container and advertises other containers as Tailscale Services based on Docker labels.

### Architecture

```
┌─────────────────────────────────────────┐
│            Docker Host                  │
│                                         │
│  ┌─────────────┐    ┌─────────────┐     │
│  │  Docktail   │    │  App        │     │
│  │             │◀───│  Container  │     │
│  │             │    │             │     │
│  │ tailscale   │    │ label:      │     │
 │  │  connected   │    │ docktail.*  │     │
│  └──────┬──────┘    └─────────────┘     │
│         │                               │
└─────────┼───────────────────────────────┘
          │
          ▼
    Tailscale Service
    (MagicDNS name)
```

### Docker Labels

```yaml
services:
  actual-budget:
    image: actualbudget/actual-server
    labels:
      - "docktail.service.enable=true"
      - "docktail.service.name=actual"
      - "docktail.service.port=5006"
```

Docktail advertises the service at `actual.tailnet.ts.net`.

### Why Docktail

- No Tailscale in every container image
- Declarative — labels define what's exposed
- Works with Komodo GitOps — labels are in compose files
- Centralized Tailscale management per Docker host
- Services get proper MagicDNS names

---

## Installation Strategy by Node Type

| Node | Tailscale Installation | Service Exposure | Exit Node |
|------|------------------------|------------------|-----------|
| **Komodo Controller** | Direct install | Advertised service (admin-only) | No |
| **ProxMox Host** | Direct install | Advertised service (admin-only) | No |
| **OCP Cluster** | Tailscale Operator | Kubernetes-native | No |
| **Docker Hosts** | Host + Docktail | Services via Docktail | No |
| **Exit Node** | Direct install | None | Yes |

### Komodo Controller (NUC)

Direct Tailscale installation. Advertises as a service for admin access only.
- Komodo UI (admin-only service)
- SSH access (admin-only)

```bash
curl -fsSL https://tailscale.com/install.sh | sh
sudo tailscale up --ssh --advertise-tags=tag:admin-infra
```

### ProxMox Host

Direct Tailscale installation. Admin-only access.
- ProxMox web UI
- SSH access to host
- VM console access

### OCP Cluster (Dev)

Uses the [Tailscale Kubernetes Operator](https://tailscale.com/kb/1236/kubernetes-operator) for Kubernetes-native integration:
- Ingress via Tailscale
- Service exposure with proper tagging
- Pod identity

### Docker Hosts (Prod/Test/Dev)

Tailscale on the host for SSH, **Docktail for service advertisement**:
- Host Tailscale provides admin SSH access
- Docktail advertises container services with appropriate tags
- Services tagged for user vs admin access

```yaml
services:
  docktail:
    image: marvinvr/docktail
    volumes:
      - /var/run/docker.sock:/var/run/docker.sock
    environment:
      - TS_AUTHKEY=${TAILSCALE_AUTHKEY}
```

### Dedicated Exit Node

A minimal node dedicated to exit node functionality:
- Tailscale with `--advertise-exit-node`
- No Docktail, no services, no subnet routes
- Provides "at home" internet access when traveling
- Isolated from internal service exposure

---

## Decisions

- **Docktail for service advertisement** — containers that need to be reachable get Docktail labels
- **ScaleTail sidecars** — per-service Tailscale sidecars for inbound and outbound tailnet access
- **Outbound-only sidecars** — lightweight sidecars for containers that need tailnet access but not inbound
- **Subnet routes are admin-only** — ACLs restrict subnet access to admin role
- **Dedicated exit node** — isolate exit node from service/subnet exposure

---

## Open Questions

- [ ] Which node becomes the dedicated exit node? (Candidate: lightweight VM or spare device)
- [ ] Tagging convention for services — how to tag user vs admin interfaces in Docktail labels?
- [ ] HA service list — which services warrant multi-node advertisement?
- [x] Grants policy — built as [[tailscale-grants]]

---

## Related Guides

- [OpenBao Tailscale Integration Guide](../../guides/openbao/tailscale-integration.md) - Secure, on-demand generation of Tailscale device auth keys for Docker containers.
