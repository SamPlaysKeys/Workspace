---
type: Note
---
# What's Next

## Recent Progress
- Overhauled UniFi firewall architecture from legacy LAN In/Out linear rules to the new Zone-Based Firewall (ZBF) matrix.
- Designed custom zones (Prod, Test, Dev, IoT, User) to map perfectly to existing VLANs to ensure the visual policy matrix remains meaningful.
- Identified and configured the correct built-in zones (External, Gateway, and Hotspot for the Guest network isolation).
- Created explicit ZBF policies for allowed paths:
  - HA to IoT management.
  - Optional User to Prod App access (for local latency bypassing Tailscale).
  - Cross-VLAN Chromecast discovery/streaming (mDNS reflector + explicit TCP ports 8008/8009).
  - Inter-environment syncs (Prod -> Test, Test -> Dev).
- Outlined explicit Drop catch-all for inter-zone isolation.
- Graduated the ZBF plan into the core documentation at `docs/homelab/network/unifi-configurations.md`.

## Next Steps
- Open UniFi Network Controller (v9.0+) and manually configure the zones and policies based on the `docs/homelab/network/unifi-configurations.md` matrix.
- Verify Home Assistant can still reach IoT devices.
- Verify phones on the `User` VLAN can cast to Chromecasts on the `IoT` VLAN.
- Verify Tailscale remains the primary overlay for administrative routing in without conflict.