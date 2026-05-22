---
type: Reference
---

# Homelab Hardware

## Overview
Inventory of physical machines, virtual machines, and network-attached storage in the homelab.

---

## Physical Machines

| Name            | Hardware Specs                                      | Role & Environment | OS      | Notes                                      |
|-----------------|-----------------------------------------------------|--------------------|---------|--------------------------------------------|
| LenovoMini 1    | 8th Gen i7, NVIDIA GPU, 16GB RAM                   | Prod Docker host   | Fedora  | Komodo periphery, Tailscale                 |
| LenovoMini 2    | 7th Gen i5, NVIDIA GPU, 16GB RAM                   | Test Docker host   | Fedora  | Komodo periphery, Tailscale                 |
| LenovoMini 3    | i5, 16GB RAM                                        | Dev (OCP node 1)   | —       | Tailscale Operator                          |
| LenovoMini 4    | i5, 16GB RAM                                        | Dev (OCP node 2)   | —       | Tailscale Operator                          |
| LenovoMini 5    | i5, 16GB RAM                                        | Dev (OCP node 3)   | —       | Tailscale Operator                          |
| JONSBO NAS      | 6th Gen i7, 16GB RAM, NVIDIA GPU, 10TB (8TB usable) | Prod NAS           | —       | Tailscale container, mapped to LenovoMini 1 |
| Intel NUC       | Celeron CPU, 8GB RAM                                | Prod management    | —       | Komodo instance, GitHub runner, Tailscale   |
| ProxMox Host    | Ryzen 3700, 16GB RAM, NVIDIA GPU                   | Mixed              | ProxMox | Tailscale, hosts VMs below                  |
| Synology NAS    | 4GB RAM, 6TB storage                                | Test NAS           | —       | Tailscale, mapped to LenovoMini 2           |
| Gaming PC       | Ryzen 5700, 16GB RAM, NVIDIA GPU                   | User (future)      | Windows | Remote Windows access                        |

---

## Virtual Machines

Hosted on **ProxMox** (Tailscale installed on ProxMox host):

| Name              | OS      | Role & Environment | Notes                                      |
|-------------------|---------|--------------------|--------------------------------------------|
| Remoting VM       | Fedora  | Remote work        | Tailscale, dotfiles, tools                 |
| Debian Test       | Debian  | Package testing    | Powered off most of the time               |
| Docker VM         | —       | Docker testing     | Komodo periphery, mirrors test/prod        |

---

## Networking
- **Firewall/Router**: Unifi Dream Machine (UDM)
- **Switch**: Unifi 16-port PoE
- **Uplink**: GFiber (1Gbps symmetrical)
- **Overlays**: Tailscale for remote management/access