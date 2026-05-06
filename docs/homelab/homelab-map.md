# Homelab Map

## Environments

| Environment | Purpose                          | Subnet      | Hardware & Services                                                  |
|-------------|----------------------------------|-------------|-----------------------------------------------------------------------|
| **Prod**    | Production services              | 10.0.1.0/24 | LenovoMini 1, JONSBO NAS, Intel NUC, Komodo, Docker workloads       |
| **Test**    | Staging and validation           | 10.0.2.0/24 | LenovoMini 2, Synology NAS, Docker VM (ProxMox), Komodo periphery   |
| **Dev**     | Development & experimentation    | 10.0.3.0/24 | LenovoMini 3–5 (OCP cluster), Tailscale Operator                     |
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
├── LenovoMini 1 (Prod Docker) → JONSBO NAS
├── LenovoMini 2 (Test Docker) → Synology NAS
├── LenovoMini 3–5 (Dev OCP cluster)
├── Intel NUC (Prod management)
├── ProxMox Host → VMs
├── Gaming PC (User)
└── IoT VLAN (Reolink, Google Home)

Tailscale Overlay: All nodes
```

---

## Services & Dependencies

**Tailscale Services** (on all Docker hosts):
- docktail (logging)
- uptimekuma (monitoring)

**Prod & Dev Services** (LenovoMini 1 & ProxMox Docker VM):
- Forgejo (Git)
- tsflow (CI/CD)
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
Intel NUC (10.0.1.0/24)
├── Komodo (orchestrates)
├── GitHub Runner (executes)
└── → Ansible/Terraform → LenovoMini 1, ProxMox, etc.

LenovoMini 1 (Prod)
├── Docker services
│   ├── komga          ├── transmission   ├── plex
│   ├── uptimekuma     ├── mongodb        ├── sonarr/radarr
│   ├── docktail       ├── miniflux       └── audiobookshelf
│   ├── Forgejo        ├── scaletail
│   ├── Signal         └── Vault/tsidp
└── JONSBO NAS (mounted)

LenovoMini 2 (Test)
├── Docker services
│   ├── komga          ├── transmission   ├── Signal
│   ├── uptimekuma     ├── mongodb        ├── Vault/tsidp
│   ├── docktail       ├── miniflux       ├── syncthing
│   └── scaletail
```

---

## Future
- Add `user` environment for Gaming PC
- Expand VLANs for guest network
- Integrate power/cooling monitoring