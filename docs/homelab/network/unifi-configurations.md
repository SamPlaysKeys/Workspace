---
type: Reference
---

# Unifi Network Configurations

## Firewall Rules

| Rule Name               | Type   | Source               | Destination          | Action | Notes                                  |
|-------------------------|--------|----------------------|-----------------------|--------|----------------------------------------|
| **IoT → Prod Block**   | LAN In | IoT (10.0.4.0/24)    | Prod (10.0.1.0/24)    | Drop   | Block cameras/devices → prod services  |
| **IoT → Test Block**   | LAN In | IoT (10.0.4.0/24)    | Test (10.0.2.0/24)    | Drop   | Block IoT → test services              |
| **IoT → Dev Block**    | LAN In | IoT (10.0.4.0/24)    | Dev (10.0.3.0/24)     | Drop   | Block IoT → OCP cluster               |
| **User → Prod Block**  | LAN In | User (10.0.10.0/24)  | Prod (10.0.1.0/24)    | Drop   | Isolate personal devices               |
| **User → Test Block**  | LAN In | User (10.0.10.0/24)  | Test (10.0.2.0/24)    | Drop   | Isolate personal devices               |
| **User → Dev Block**   | LAN In | User (10.0.10.0/24)  | Dev (10.0.3.0/24)     | Drop   | Isolate personal devices               |
| **Prod → Test Permit** | LAN In | Prod (10.0.1.0/24)   | Test (10.0.2.0/24)    | Accept | Allow prod → test syncs                |
| **Test → Dev Permit**  | LAN In | Test (10.0.2.0/24)   | Dev (10.0.3.0/24)     | Accept | Allow test → dev validations           |

---

## WiFi Networks

| SSID      | VLAN | Security       | Band Steering | Fast Roaming | Client Isolation |
|-----------|------|----------------|----------------|--------------|-----------------|
| **Home**  | 10   | WPA3           | Prefer 5GHz    | Enabled      | Disabled          |
| **IoT**   | 4    | WPA2           | Band Steer     | Disabled     | Enabled           |
| **Guest** | 20   | WPA2 (future)  | Prefer 5GHz    | Disabled     | Enabled           |

---

## Port Profiles

| Profile Name          | Native Network | Tagged VLANs               | Notes                          |
|-----------------------|----------------|----------------------------|--------------------------------|
| **Docker Hosts**     | Prod (1)      | Test (2), Dev (3)          | LenovoMini 1–2, ProxMox Docker |
| **OCP Nodes**        | Dev (3)       | —                          | LenovoMini 3–5                |
| **Management**       | Prod (1)      | —                          | Intel NUC, ProxMox host       |
| **IoT**             | IoT (4)       | —                          | Reolink, Google Home          |

---

## DHCP

| VLAN          | DHCP Range       | Lease Time | DNS Servers          | Notes                          |
|---------------|-------------------|------------|-----------------------|--------------------------------|
| Prod (1)     | 10.0.1.100–200   | 24h        | 10.0.1.5, 1.1.1.1    | DHCP reservations for servers  |
| Test (2)     | 10.0.2.100–200   | 24h        | 10.0.2.1, 1.1.1.1    |                                |
| Dev (3)      | 10.0.3.100–200   | 24h        | 10.0.3.1, 1.1.1.1    |                                |
| IoT (4)      | 10.0.4.100–150   | 12h        | 10.0.4.1, 1.1.1.1    | Static leases for cameras      |
| User (10)    | 10.0.10.100–250  | 12h        | 10.0.10.1, 1.1.1.1   |                                |

---

## Future
- **Guest Network**: Enable when needed on 10.0.20.0/24
- **SNMP Monitoring**: Configure UDM Pro for observability