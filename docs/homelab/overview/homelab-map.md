# Homelab Map

## Environments

| Environment | Purpose                          | Subnet      | Hardware & Services                                                  |
|-------------|----------------------------------|-------------|-----------------------------------------------------------------------|
| **Prod**    | Production services              | 10.0.1.0/24 | LenovoMini 1, UnRaid NAS, Intel NUC, Komodo, Docker workloads       |
| **Test**    | Staging and validation           | 10.0.2.0/24 | LenovoMini 2, Synology NAS, Komodo periphery                         |
| **Dev**     | Development & experimentation    | 10.0.3.0/24 | ProxMox Host (DevDocker VM, DevNode VMs), LenovoMini 3–5 (OCP cluster), Tailscale Operator |
| **User**    | Personal workstation             | 10.0.10.0/24 | Gaming PC                                                              |
| **IoT**     | Smart home & cameras             | 10.0.4.0/24 | Reolink cameras, Google Home                                          |

---

## VLANs & Subnets

| VLAN | Description               | Subnet      | Firewall Rules                          | Unifi SSID / Notes               |
|------|---------------------------|-------------|------------------------------------------|-----------------------------------|
| 1    | Prod                      | 10.0.1.0/24 | Allow all internal; restrict IoT → Prod | Wired only                          |
| 2    | Test                      | 10.0.2.0/24 | Allow all internal; restrict IoT → Test | Wired only                          |
| 3    | Dev                       | 10.0.3.0/24 | Allow all internal                       | Wired only                          |
| 4    | IoT                       | 10.0.4.0/24 | Isolate from prod/test/dev               | `IoT` (WPA2)                     |
| 10   | User                      | 10.0.10.0/24| Isolate from prod/test/dev/IoT           | `Home` (WPA3)                      |

---

## Network Topology
```
GFiber (1Gbps) → Unifi UDM → Unifi 16-port PoE Switch
│
├── LenovoMini 1 (Prod Docker) → UnRaid NAS
├── LenovoMini 2 (Test Docker) → Synology NAS
├── ProxMox Host (Dev) → DevDocker VM, DevNode VMs
├── LenovoMini 3–5 (Dev OCP cluster)
├── Intel NUC (Prod management / Komodo Controller)
├── Gaming PC (User)
└── IoT VLAN (Reolink, Google Home)

Tailscale Overlay: All nodes
```

---

## Services & Dependencies

**Tailscale Services** (on all Docker hosts):
- docktail (logging)
- uptimekuma (monitoring)

**Dev Services** (ProxMox DevDocker VM):
- Forgejo (Git)
- tsflow (CI/CD)
- Experimental containers

**Prod Services shared with Dev** (LenovoMini 1 & ProxMox DevDocker VM):
- komga (media)

**Test & Prod Services** (LenovoMini 1 & LenovoMini 2):
- Signal (chat)
- Vault (secrets)
- tsidp (identity)
- syncthing (file sync)
- scaletail (logging)
- miniflux (RSS)
- transmission (torrent)
- mongodb (database)

**Prod-Only Services** (LenovoMini 1):
- -arr suite (media automation: Sonarr, Radarr, etc.)
- plex (media server)
- audiobookshelf (audiobook management)

```
Intel NUC (10.0.1.0/24) - Komodo Controller
├── Komodo Core (orchestrates)
├── GitHub Runner (executes)
└── → Ansible/Terraform → LenovoMini 1, LenovoMini 2, ProxMox

LenovoMini 1 (Prod)
├── Periphery Agent
├── Docker services
│   ├── komga          ├── transmission   ├── plex
│   ├── uptimekuma     ├── mongodb        ├── sonarr/radarr
│   ├── docktail       ├── miniflux       └── audiobookshelf
│   ├── Signal         ├── scaletail
│   └── Vault/tsidp
└── UnRaid NAS (mounted)

LenovoMini 2 (Test)
├── Periphery Agent
├── Docker services
│   ├── komga          ├── transmission   ├── Signal
│   ├── uptimekuma     ├── mongodb        ├── Vault/tsidp
│   ├── docktail       ├── miniflux       ├── syncthing
│   └── scaletail
└── Synology NAS (mounted)

ProxMox Host (Dev)
├── DevDocker VM
│   ├── Periphery Agent
│   ├── Forgejo (Git)
│   ├── tsflow (CI/CD)
│   └── Experimental containers
└── DevNode VMs (unmanaged sandbox)
```

---

## Future
- **Guest Network**: VLAN (10.0.20.0/24) on Unifi SSID `Guest`
- **Tailscale SSH**: Secure remote CLI access to all nodes

---

## Observability (in progress)

Goals and acceptance criteria: [observability/status-and-operator-dashboard.md](../observability/status-and-operator-dashboard.md) (see also [observability/README.md](../observability/README.md)).

| Initiative | Purpose |
|------------|---------|
| **Public status** | `status.samplayskeys.com` on a VPS — user-facing service health (non-admin), available during local outages. |
| **Operator dashboard** | Single pane (e.g. Glances ± link portal) with lab overview and links to Uptime Kuma, Komodo, and management UIs. |