---
type: ADR
layout: page
title: 'Decision: Smart Switch Protocol'
category: Homelab
status: Active
---


# Decision: Smart Switch Protocol

**Date:** 2026-05-11  
**Status:** Decided  
**Decision:** Zigbee (Inovelli Blue switches)

---

## Context

Planning IoT lighting for new house. Requirements:
- Work in 3-way configurations with only ONE switch replaced
- Support wireless buttons for remote control
- Prioritize cost savings and upgradeability
- Integrate with self-hosted Home Assistant

Neutral wire is present.

---

## Options Considered

### Zigbee — Inovelli Blue Series

**Protocol:** Zigbee 3.0  
**Switch Cost:** ~$45-50  
**Coordinator Cost:** ~$20-30

**Pros:**
- Works with existing dumb switch in 3-way config
- Large device ecosystem (sensors, buttons, plugs)
- Cheap wireless buttons ($7-15)
- Direct Zigbee binding (button controls switch without hub)
- Configurable LED notification bar
- Scene/multi-tap support
- Mesh network extends coverage

**Cons:**
- Higher per-switch cost
- 2.4GHz interference possible (manageable)

---

### Z-Wave — Zooz Switches

**Protocol:** Z-Wave Plus  
**Switch Cost:** ~$30-35  
**Coordinator Cost:** ~$35-45

**Pros:**
- Works with existing dumb switch in 3-way config
- Lower per-switch cost
- Dedicated frequency (no WiFi interference)
- Mature, reliable protocol

**Cons:**
- Smaller device ecosystem
- Wireless buttons limited and expensive ($40+)
- Less feature-rich

---

### Also Considered

**Lutron Caseta:** Excellent reliability, but requires proprietary bridge. Less flexible ecosystem.

**WiFi switches:** Cloud dependency or complex local setup. Doesn't align with self-hosted goals.

---

## Decision

**Zigbee with Inovelli Blue switches.**

### Rationale

1. **Accessory ecosystem:** Zigbee buttons and sensors are significantly cheaper. $7 IKEA button vs $40+ Z-Wave compounds quickly.

2. **Upgradeability:** Largest device selection across manufacturers for future expansion.

3. **Feature set:** LED notifications, direct binding, scene control — useful for automations.

4. **3-way handling:** Single switch replacement works cleanly.

---

## Implementation

- **Coordinator:** Sonoff Zigbee 3.0 USB Dongle Plus
- **Integration:** ZHA or Zigbee2MQTT (both well-supported)
- **Placement:** Central location or USB extension to avoid interference
