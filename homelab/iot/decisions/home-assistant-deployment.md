---
type: ADR
layout: page
title: 'ADR: Home Assistant Deployment'
category: Homelab
status: Active
---


# ADR: Home Assistant Deployment

**Date:** 2026-05-11  
**Status:** Accepted  
**Deciders:** Self

---

## Context

Home Assistant (HA) is the central hub for IoT automation in the new house. It needs to integrate with:

- Zigbee devices (switches, sensors, buttons) via USB coordinator
- Tapo cameras (RTSP/Tapo integration)
- Unifi Protect (running on UDM)
- Google Home (TTS announcements)
- Sensi thermostats (cloud API)

The homelab has a defined Prod/Test/Dev architecture with Komodo-managed Docker hosts. Home Assistant must fit into this architecture while meeting its unique requirements.

### Requirements

1. **USB access** — Zigbee coordinator must be physically attached to host
2. **Network access** — Must reach IoT VLAN (10.0.4.0/24) for cameras, Google Home, etc.
3. **Unifi Protect access** — Must reach UDM for Protect integration
4. **Reliability** — Automation failures directly impact daily life
5. **Manageability** — Should fit into existing GitOps/Komodo workflow where possible
6. **Backup/Recovery** — Must be recoverable quickly if host fails

### Constraints

- IoT VLAN (10.0.4.0/24) is isolated from other VLANs by design
- Zigbee coordinator is USB-bound — cannot easily fail over to another host
- Home Assistant does not support native clustering/HA

---

## Decision

**Deploy Home Assistant as a container on LenovoMini 1 (Prod Docker host) with USB passthrough for the Zigbee coordinator.**

### Phased Approach

**Phase 1 — Bootstrap (RPi):**
- Run HA on existing Raspberry Pi 4
- Place on Prod VLAN (10.0.1.0/24)
- Configure firewall rules for IoT access
- Validates the setup before full integration

**Phase 2 — Production Migration:**
- Migrate HA to container on LenovoMini 1
- USB passthrough for Zigbee coordinator
- Komodo-managed deployment
- Automated backups to UnRaid NAS

---

## Options Considered

### Option 1: Dedicated Raspberry Pi

**Description:** Run Home Assistant OS on a dedicated RPi 4.

**Pros:**
- Simple setup
- Direct USB access (no passthrough complexity)
- Isolated from other workloads

**Cons:**
- Another device to manage outside Komodo
- SD card reliability concerns
- Limited compute for future expansion (Frigate, etc.)
- No integration with existing backup infrastructure

**Verdict:** Good for bootstrap, not ideal long-term.

---

### Option 2: Container on Prod Docker Host (LenovoMini 1)

**Description:** Run HA as a Docker container on the existing Prod host, managed via Komodo.

**Pros:**
- Better hardware (8th Gen i7, 16GB RAM, NVIDIA GPU)
- Integrated with existing management (Komodo, GitOps)
- Backups to UnRaid NAS
- Network already on Prod VLAN with access to other services
- GPU available for future Frigate integration

**Cons:**
- USB passthrough required (adds configuration)
- Shares resources with Plex/*arr (mitigated by resource limits)
- Single point of failure (mitigated by backups)

**Verdict:** Best fit for long-term architecture.

---

### Option 3: VM on ProxMox

**Description:** Run HA in a dedicated VM on the ProxMox host.

**Pros:**
- Isolated resources
- ProxMox has snapshot/backup features

**Cons:**
- ProxMox is on Dev VLAN (10.0.3.0/24) — wrong network segment
- USB passthrough to VM is more complex than Docker
- Not on the Komodo-managed path

**Verdict:** Network placement is wrong. Rejected.

---

### Option 4: Dedicated Mini Host on Prod VLAN

**Description:** Purchase a dedicated small device (NUC, mini PC) for HA.

**Pros:**
- Dedicated to home automation
- Direct USB, proper network placement

**Cons:**
- Additional hardware cost
- Another device outside existing management
- Overkill given available capacity on LenovoMini 1

**Verdict:** Unnecessary given existing infrastructure.

---

## Consequences

### Positive

- **Integrated management:** HA deployment is Komodo-managed, follows GitOps patterns
- **Better hardware:** More headroom for future expansion (Frigate, more integrations)
- **Unified backups:** HA config/data backed up to UnRaid alongside other Prod data
- **Network positioning:** Prod VLAN has (or will have) appropriate access to IoT and UDM

### Negative

- **USB passthrough complexity:** Requires privileged container or explicit device mapping
- **Shared host:** HA competes with Plex/*arr for resources (mitigated with resource limits)
- **Single point of failure:** If LenovoMini 1 goes down, HA is down (mitigated by backups + quick restore)

### Neutral

- **No native HA failover:** Accepted limitation — most homelabs operate this way
- **Zigbee bound to host:** Coordinator cannot fail over, but Zigbee direct bindings provide local fallback

---

## Implementation

### Phase 1: Bootstrap on RPi

**Timeline:** Immediate (while homelab is being set up)

1. Install Home Assistant OS on Raspberry Pi 4
2. Connect Zigbee coordinator (Sonoff dongle)
3. Place RPi on Prod VLAN (10.0.1.0/24)
4. Configure firewall rules:
   - Allow Prod → IoT (for cameras, Google Home)
   - Allow Prod → UDM management interface (for Unifi Protect)
5. Set up integrations (Zigbee, Tapo, Protect, Google Cast, Sensi)
6. Configure automations
7. Validate everything works

**Backup:** Manual snapshots to USB or network share

---

### Phase 2: Migrate to Prod Docker

**Timeline:** When homelab Prod environment is stable

1. **Prepare LenovoMini 1:**
   - Identify USB port for Zigbee coordinator
   - Test USB device passthrough (`/dev/ttyUSB0` or `/dev/ttyACM0`)

2. **Create HA container configuration:**

   ```yaml
   # docker-compose.yml (Komodo-managed)
   services:
     homeassistant:
       container_name: homeassistant
       image: ghcr.io/home-assistant/home-assistant:stable
       restart: unless-stopped
       privileged: true  # Required for USB and network discovery
       network_mode: host  # Simplifies discovery protocols
       volumes:
         - /opt/homeassistant/config:/config
         - /etc/localtime:/etc/localtime:ro
       devices:
         - /dev/ttyUSB0:/dev/ttyUSB0  # Zigbee coordinator
       environment:
         - TZ=America/New_York
   ```

   **Notes:**
   - `privileged: true` or explicit device mapping required for USB
   - `network_mode: host` recommended for mDNS/SSDP discovery (Google Cast, etc.)
   - Adjust device path based on actual Zigbee dongle

3. **Migrate configuration:**
   - Create HA snapshot on RPi
   - Restore snapshot to container config volume
   - Verify Zigbee coordinator is detected
   - Verify all integrations reconnect

4. **Set up backups:**
   - Mount UnRaid NAS share for backup storage
   - Configure HA to snapshot to NAS (daily)
   - Or use Komodo/external script to backup `/opt/homeassistant/config`

5. **Add to Komodo:**
   - Create TOML resource for HA container
   - Enable GitOps management
   - Set `deploy: false` initially (manual approval for HA changes)

6. **Decommission RPi:**
   - Verify Prod deployment is stable (run parallel for a few days if desired)
   - Power down RPi
   - Keep as cold spare for emergency restore

---

### Network Configuration

**Firewall rules needed (on UDM):**

| Source | Destination | Port | Purpose |
|--------|-------------|------|---------|
| Prod VLAN (10.0.1.0/24) | IoT VLAN (10.0.4.0/24) | * | HA reaching IoT devices |
| Prod VLAN | UDM management | 443, 7443 | Unifi Protect API |
| IoT VLAN | Prod VLAN:8123 | 8123 | Optional: device callbacks to HA |

**Note:** IoT VLAN should remain isolated from other VLANs. Only Prod (where HA lives) gets access.

---

### Backup & Recovery

**Backup strategy:**
- Daily HA snapshots to UnRaid NAS
- Retain 7 daily + 4 weekly snapshots
- Config directory also backed up via standard Prod backup job

**Recovery procedure:**
1. If LenovoMini 1 fails:
   - Option A: Restore to LenovoMini 2 (Test) temporarily
   - Option B: Boot RPi with latest snapshot
2. Restore latest snapshot
3. Reconnect Zigbee coordinator (may need re-pairing if coordinator changed)
4. Verify automations

**RTO target:** < 1 hour (acceptable for home automation)

---

### Resilience Notes

**What survives HA downtime:**
- Zigbee direct bindings (button → switch) work without HA
- Smart switches work manually (physical toggle)
- Cameras continue recording (Protect is independent)
- Thermostats continue on last schedule (Sensi is independent)

**What fails:**
- Automations (door alerts, scenes, presence-based lighting)
- Voice control via HA (Google Home native commands still work)
- Unified dashboard/control

**Acceptable trade-off:** Brief outages are tolerable. Critical functions (lights, climate) have manual fallback.

---

---

## Phase 3: Automated Failover (Stretch Goal)

**Status:** Future consideration  
**Complexity:** Medium-high  
**Additional hardware:** Second Zigbee coordinator (~$25)

### Concept

Implement a warm/cold spare HA instance on the Test environment (LenovoMini 2) with automated failover capability. When the primary (Prod) instance has unfixable issues, a single-click remediation brings up the spare and updates network rules.

### Architecture

```
                     Normal Operation
┌─────────────────────────────────────────────────────────────┐
│                                                             │
│   Prod VLAN (10.0.1.0/24)              IoT VLAN (10.0.4.0/24)
│   ┌─────────────────────┐              ┌─────────────────┐  │
│   │   LenovoMini 1      │   allowed    │  Zigbee devices │  │
│   │   [HA - Active]     │─────────────▶│  Cameras        │  │
│   │   [Zigbee Coord 1]  │              │  Google Home    │  │
│   └─────────────────────┘              └─────────────────┘  │
│                                                 ▲           │
│   Test VLAN (10.0.2.0/24)               blocked │           │
│   ┌─────────────────────┐                       │           │
│   │   LenovoMini 2      │───────────────────────┘           │
│   │   [HA - Standby]    │                                   │
│   │   [Zigbee Coord 2]  │                                   │
│   └─────────────────────┘                                   │
│                                                             │
└─────────────────────────────────────────────────────────────┘

                     Failover State
┌─────────────────────────────────────────────────────────────┐
│                                                             │
│   Prod VLAN (10.0.1.0/24)              IoT VLAN (10.0.4.0/24)
│   ┌─────────────────────┐              ┌─────────────────┐  │
│   │   LenovoMini 1      │   blocked    │  Zigbee devices │  │
│   │   [HA - DOWN]       │──────────X   │  Cameras        │  │
│   │                     │              │  Google Home    │  │
│   └─────────────────────┘              └─────────────────┘  │
│                                                 ▲           │
│   Test VLAN (10.0.2.0/24)               allowed │           │
│   ┌─────────────────────┐                       │           │
│   │   LenovoMini 2      │───────────────────────┘           │
│   │   [HA - Active]     │                                   │
│   │   [Zigbee Coord 2]  │                                   │
│   └─────────────────────┘                                   │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

### Components

**1. Standby HA Instance (LenovoMini 2)**
- Home Assistant container (same config as Prod)
- Second Zigbee coordinator (Sonoff dongle)
- Container stopped or running in standby mode

**2. Config Synchronization**
- Option A: Scheduled rsync of `/opt/homeassistant/config` from Prod → Test (via NAS or direct)
- Option B: Shared config on NAS mount (both read, only active writes)
- Option C: Git-based config with HA Git integration

**3. Zigbee Network Portability**
- Use Zigbee2MQTT (not ZHA) — stores network config in `coordinator_backup.json`
- Backup can be restored to second coordinator
- Devices don't need re-pairing if network key is preserved
- **Caveat:** Second coordinator must be same model or compatible

**4. Firewall Automation (Ansible/Terraform)**

```yaml
# ansible playbook: failover-to-test.yml
- name: Failover HA to Test environment
  hosts: localhost
  tasks:
    - name: Update UDM firewall - block Prod to IoT
      community.general.udm_firewall_rule:
        name: "Prod-to-IoT"
        state: disabled
      
    - name: Update UDM firewall - allow Test to IoT
      community.general.udm_firewall_rule:
        name: "Test-to-IoT"
        state: enabled
        
    - name: Start HA container on Test
      community.docker.docker_container:
        name: homeassistant
        state: started
      delegate_to: lenovomini2
      
    - name: Stop HA container on Prod (if reachable)
      community.docker.docker_container:
        name: homeassistant
        state: stopped
      delegate_to: lenovomini1
      ignore_errors: yes
```

**5. Failback Procedure**
- Reverse the process when Prod is healthy
- Sync any config changes from Test back to Prod
- Re-enable Prod firewall rules, disable Test

### Spare Modes

| Mode | Standby State | Sync Frequency | Failover Time | Resource Usage |
|------|---------------|----------------|---------------|----------------|
| **Cold Spare** | Container stopped | Daily backup restore | ~5-10 min | Minimal |
| **Warm Spare** | Container running (no IoT access) | Near real-time rsync | ~1-2 min | Moderate |

**Recommendation:** Start with Cold Spare. Warm Spare adds complexity for marginal time savings.

### Triggering Failover

**Manual (Phase 3a):**
- Single-click: Run Ansible playbook from Komodo UI or CLI
- Human detects issue, initiates failover

**Automated (Phase 3b, future):**
- Health check monitors HA (e.g., `/api/` endpoint)
- Consecutive failures trigger alert
- Optional: Auto-failover after N failures (risky, needs tuning)

### Prerequisites

Before implementing:

1. **Hardware:** Second Zigbee coordinator (~$25)
2. **Network:** Test VLAN firewall rules pre-configured (disabled)
3. **Ansible:** Playbooks for failover/failback
4. **UDM API access:** Ansible collection for Unifi or direct API calls
5. **Zigbee2MQTT:** Migrate from ZHA if using ZHA initially
6. **Testing:** Validate failover in controlled scenario

### Risks & Mitigations

| Risk | Mitigation |
|------|------------|
| Zigbee devices don't reconnect to second coordinator | Use Zigbee2MQTT with network backup; same coordinator model |
| Config drift between Prod and Test | Automated sync + Git-based config |
| Failover triggered unnecessarily | Manual trigger only (Phase 3a); conservative health checks (Phase 3b) |
| Firewall automation fails | Pre-test playbooks; manual fallback documented |

### Cost

| Item | Cost |
|------|------|
| Second Zigbee coordinator | ~$25 |
| Time to implement | Medium |
| Ongoing maintenance | Low (once stable) |

### Decision

**Defer to Phase 3.** Implement after Phase 2 is stable and proven. The complexity is justified only after experiencing reliability issues or when uptime requirements increase.

---

## References

- [Home Assistant Container Installation](https://www.home-assistant.io/installation/linux#docker-compose)
- [Zigbee2MQTT Docker](https://www.zigbee2mqtt.io/guide/installation/02_docker.html)
- [Zigbee2MQTT Coordinator Backup](https://www.zigbee2mqtt.io/guide/usage/backup_restore.html)
- [Home Assistant Backup/Restore](https://www.home-assistant.io/common-tasks/os/#backups)
- [Unifi API](https://ubntwiki.com/products/software/unifi-controller/api)
