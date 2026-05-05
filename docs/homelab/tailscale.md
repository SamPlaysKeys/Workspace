# Tailscale

Network connectivity strategy for the homelab using Tailscale mesh VPN.

---

## What is Tailscale?

Tailscale is a mesh VPN built on WireGuard that creates a secure network (called a "tailnet") across devices regardless of their physical location. Unlike traditional VPNs, there's no central server that all traffic flows through — devices connect directly to each other.

Key concepts:
- **Tailnet** — Your private network of connected devices
- **MagicDNS** — Automatic DNS for devices (e.g., `device-name.tailnet-name.ts.net`)
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

## Use Cases

### Remote Application Access (Actual Budget)

Actual Budget (self-hosted budgeting app) can be accessed remotely via Tailscale without exposing it to the internet.

```
Phone/Laptop ──(Tailscale)──▶ actual.tailnet.ts.net:5006 ──▶ Actual Budget container
```

- No public exposure
- Accessible from any device on the tailnet
- Partner can access with their own Tailscale identity (see Shared Access below)

### Remote Device Access

**Tailscale SSH:**
- SSH into any device on the tailnet without managing SSH keys across devices
- Tailscale handles authentication via identity provider
- `ssh user@device.tailnet.ts.net`

**RDP (Windows/Linux desktops):**
- Access desktop machines remotely via RDP over Tailscale
- No port forwarding required
- Encrypted end-to-end by WireGuard

### Remote File Access

**SMB shares:**
- Access Windows/Samba shares from anywhere
- `\\device.tailnet.ts.net\share` or `smb://device.tailnet.ts.net/share`
- Works from phone apps that support SMB

**NFS mounts:**
- Mount NFS shares over Tailscale
- Useful for accessing media libraries, backups, etc.
- May have performance implications over high-latency connections

---

## Shared Access (Partner)

Partner has her own Tailscale account and devices, shared into the tailnet with controlled access.

### Adversarial Access Model

Treat partner's access as **adversarial by default** — not because of distrust, but because:
- Her devices may be compromised
- Mistakes happen (accidental deletion, misconfiguration)
- Principle of least privilege — only grant what's needed

### Access Tiers

| Resource | Partner Access | Rationale |
|----------|----------------|-----------|
| Actual Budget | Yes | Shared finances |
| Media (Plex, etc.) | Yes | Shared entertainment |
| File shares (photos, documents) | Limited folders | Only shared content |
| Admin interfaces (Komodo, Proxmox) | No | No need for management access |
| SSH/RDP to servers | No | No need for direct machine access |
| Dev environments | No | Personal work tools |

### Implementation (Tailscale ACLs)

```json
{
  "acls": [
    // Partner can access specific services
    {
      "action": "accept",
      "src": ["group:partner"],
      "dst": [
        "actual-budget:5006",
        "plex:32400",
        "nas:445"  // SMB, limited shares via Samba config
      ]
    },
    // Partner cannot access management
    {
      "action": "deny",
      "src": ["group:partner"],
      "dst": ["tag:management"]
    }
  ]
}
```

### Environment-Based Protections

| Environment | Partner Access |
|-------------|----------------|
| Prod | Limited (specific apps only) |
| Test | No |
| Dev | No |
| DevOCP | No |
| DevNode | No |

Prod services exposed to partner are explicitly whitelisted. Everything else is deny-by-default.

---

## Docktail: Container Exposure via Docker Labels

[Docktail](https://github.com/docktail/docktail) runs as a sidecar container and exposes other containers to Tailscale based on Docker labels. This avoids installing Tailscale directly in every container.

### How It Works

```
┌─────────────────────────────────────────┐
│            Docker Host                  │
│                                         │
│  ┌─────────────┐    ┌─────────────┐     │
│  │  Docktail   │    │  App        │     │
│  │  (sidecar)  │◀───│  Container  │     │
│  │             │    │             │     │
│  │ tailscale   │    │ label:      │     │
│  │ connected   │    │ ts.enable   │     │
│  └──────┬──────┘    └─────────────┘     │
│         │                               │
└─────────┼───────────────────────────────┘
          │
          ▼
      Tailnet
```

### Docker Labels

```yaml
services:
  actual-budget:
    image: actualbudget/actual-server
    labels:
      - "docktail.enable=true"
      - "docktail.hostname=actual"
      - "docktail.port=5006"
```

Docktail picks up the labels and exposes the service at `actual.tailnet.ts.net:5006`.

### Advantages

- No Tailscale in every container image
- Declarative — labels define what's exposed
- Works with Komodo GitOps — labels are in compose files
- Centralized Tailscale management per Docker host

---

## Installation Strategy by Node Type

| Node | Tailscale Installation | Service Exposure |
|------|------------------------|------------------|
| **Komodo Controller** | Direct install | Direct (management access) |
| **ProxMox Host** | Direct install | Direct (VM management, SSH) |
| **OCP Cluster** | Tailscale Operator | Kubernetes-native exposure |
| **Docker Hosts (Prod/Test/Dev)** | Host has Tailscale | Services via Docktail |

### Komodo Controller (NUC)

Direct Tailscale installation on the host. Exposes:
- Komodo UI
- SSH access

```bash
curl -fsSL https://tailscale.com/install.sh | sh
sudo tailscale up --ssh
```

### ProxMox Host

Direct Tailscale installation. Exposes:
- ProxMox web UI
- SSH access to host
- Access to VM consoles

### OCP Cluster (DevOCP)

Uses the [Tailscale Kubernetes Operator](https://tailscale.com/kb/1236/kubernetes-operator). This integrates with Kubernetes-native concepts:
- Ingress via Tailscale
- Service exposure
- Pod identity

```yaml
apiVersion: tailscale.com/v1alpha1
kind: Connector
metadata:
  name: ts-connector
spec:
  hostname: ocp-cluster
```

### Docker Hosts (Prod/Test/Dev MiniPCs, DevDocker VM)

Tailscale installed on the host, but **services are NOT exposed directly**. Instead:
- Docktail container runs on each Docker host
- Services opt-in to Tailscale exposure via Docker labels
- Host Tailscale provides SSH access to the host itself

```yaml
# Part of each Docker host's base stack
services:
  docktail:
    image: docktail/docktail
    volumes:
      - /var/run/docker.sock:/var/run/docker.sock
    environment:
      - TS_AUTHKEY=${TAILSCALE_AUTHKEY}
```

---

## Open Questions

- [ ] Docktail vs Tailscale sidecar containers — which is more maintainable?
- [ ] Funnel for truly public services (e.g., public blog) or keep everything behind Tailscale?
- [ ] Subnet routing for accessing non-Tailscale devices on home network?
- [ ] Exit node for routing all traffic through homelab when traveling?
