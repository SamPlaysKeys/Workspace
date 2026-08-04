---
type: ADR
layout: page
title: 'Decision: Door Locks'
category: Homelab
status: Active
---
{% raw %}


# Decision: Door Locks

**Date:** 2026-05-11  
**Status:** Deferred  
**Decision:** No smart locks initially; rely on physical keys + camera detection

---

## Context

Evaluating whether to include smart locks in the initial IoT deployment.

---

## Decision

**Defer smart locks. Focus on detection, not prevention.**

### Rationale

1. **Security philosophy:** If someone is determined to get in, they will. Detection and evidence (cameras) are more valuable than a smart lock.

2. **Existing coverage:** Unifi Protect cameras + door sensors provide detection and alerting.

3. **Complexity trade-off:** Smart locks add battery management, failure modes, and integration points without a clear use case.

4. **Physical keys work:** No current pain point that a smart lock solves.

---

## Future Considerations

Revisit when one of these use cases becomes valuable:
- Keyless entry for family
- Temporary access codes for guests/contractors
- Auto-lock after X minutes
- Remote verification
- Presence-based auto-unlock

### Options to Evaluate Later

**Consumer (HA compatible):**
- Yale Assure Lock 2 (Zigbee, Z-Wave, Matter)
- Schlage Encode Plus (WiFi, HomeKit)
- August Wi-Fi Smart Lock (retrofit)
- Level Lock+ (invisible, HomeKit/Matter)

**Enterprise:**
- Unifi Access — NFC/card based, integrates with Protect
- Higher cost (~$300+/lock + hub), more commercial-grade
- Interesting for "set and forget" but overkill for residential

### When Adding Locks

1. Prefer Zigbee or Matter (aligns with existing protocol)
2. Integrate via Home Assistant
3. Automate: auto-lock, lock-on-leave, alert if unlocked while away
4. Tie to camera snapshots on unlock events

{% endraw %}