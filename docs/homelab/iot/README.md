# IoT Plan — New House

Overview and project plan for IoT deployment at the new house.

**Philosophy:** Convenience first, security second. Self-hosted, local control where possible.

---

## Platform Stack

| Layer | Choice | Notes |
|-------|--------|-------|
| **Hub** | Home Assistant OS | Running on Raspberry Pi 4 (migrate to homelab later) |
| **Primary Protocol** | Zigbee | Via Sonoff Zigbee 3.0 coordinator |
| **Voice** | Google Home | Existing ecosystem, TTS announcements via HA |
| **Network** | Unifi Network | Already deployed |
| **Cameras** | Unifi Protect + Tapo | 3x Tapo (garage, living room, study) + existing Protect |
| **Thermostats** | Sensi (2 zones) | Cloud integration via HA (acceptable trade-off) |

---

## Phase 1 — Initial Deployment

### Target Rooms

**Garage:**
- 1x smart switch (single, not 3-way)
- 1x wireless button (control from adjacent room)

**Bedroom:**
- 4-gang panel, 3 smart + 1 dumb
- Fan and light on fan are separately wired

| Position | Device | Controls |
|----------|--------|----------|
| 1 | Inovelli Blue 2-1 (on/off) | Hall light (3-way) |
| 2 | Inovelli Blue Fan Switch | Fan motor |
| 3 | Inovelli Blue 2-1 (dimmer) | Fan's light |
| 4 | Dumb Decora switch | Alcove light |

**Whole House:**
- Door sensors on 4 doors (front, back, office, garage entry)
- Automations for "door left open" alerts via Google TTS

### Shopping List

| Item | Qty | ~Cost | Notes |
|------|-----|-------|-------|
| Sonoff Zigbee 3.0 USB Dongle Plus | 1 | $25 | Coordinator for HA |
| Inovelli Blue 2-1 Switch | 3 | $135 | Hall 3-way, garage, fan light (dimmer) |
| Inovelli Blue Fan Switch | 1 | $45 | Bedroom fan motor |
| Zigbee wireless button | 1 | $12 | IKEA Shortcut or Aqara |
| Zigbee door sensors | 4 | $50 | Aqara or Sonoff SNZB-04 |
| Decora 4-gang wall plate | 1 | $5 | Bedroom panel (replacing toggle) |
| **Total** | | **~$270-275** | |

**Already owned:**
- Raspberry Pi 4 (for Home Assistant)
- Tapo cameras (3x)
- Unifi Protect + cameras
- Sensi thermostats (2x)
- LiftMaster garage door openers (2x, staying on myQ for now)

### Switch Notes

- **Inovelli Blue 2-1** is configurable as on/off OR dimmer (same hardware)
- **3-way configuration:** Replace only ONE switch, existing dumb switch stays
- **Neutral wire:** Confirmed present
- **Style:** Decora (rectangular rocker) — replace existing toggle plate

---

## Deferred Items

| Item | Reason | Future Path |
|------|--------|-------------|
| ratgdo (garage doors) | Not a priority | Add when local control becomes valuable (~$35-50/unit) |
| Smart locks | Detection > prevention | Evaluate Zigbee/Matter locks or Unifi Access later |
| Unifi Access | Cost | Revisit in a few years if "set and forget" lock system needed |

See [decisions/](decisions/) for detailed rationale.

---

## Integrations

| System | Integration Method | Local? |
|--------|-------------------|--------|
| Zigbee devices | ZHA or Zigbee2MQTT | Yes |
| Google Home | Google Cast integration | TTS only (cloud) |
| Tapo cameras | Tapo integration or RTSP | RTSP is local |
| Unifi Protect | Native HA integration | Yes |
| Sensi thermostats | Emerson Sensi integration | No (cloud API) |
| LiftMaster (myQ) | Not integrated | N/A (deferred) |

---

## Key Automations

See [automations.md](automations.md) for full workflow definitions.

| Automation | Trigger | Action |
|------------|---------|--------|
| Door left open | Door open > 5 min | TTS to Google Home |
| Door reminder | Still open | Repeat TTS every 5 min |
| Garage night alert | Garage door opens 10PM-6AM | TTS + camera snapshot |
| Garage button | Zigbee button press | Toggle garage light |
| Goodnight scene | Button or voice | Hall off, bedroom dims, fan on |
| Arriving home | Person arrives after sunset | Entry lights on |

---

## Network Considerations

- Zigbee coordinator should be central in house (or use USB extension cable to avoid USB 3.0 interference)
- Each mains-powered Zigbee device (switches) extends the mesh
- Consider IoT VLAN for Tapo cameras and other cloud-dependent devices

---

## Future Expansion

When ready to grow:

- **More rooms:** Add Inovelli switches as needed (same ecosystem)
- **Motion sensors:** Zigbee PIR sensors for presence-based lighting
- **Water leak sensors:** Zigbee leak detectors for bathrooms, laundry, water heater
- **Frigate:** Local AI object detection when homelab is running (needs Coral TPU)
- **ratgdo:** Local garage door control when automation use case emerges
- **Locks:** Zigbee/Matter locks or Unifi Access when keyless entry becomes valuable

---

## References

- [Inovelli Blue Series](https://inovelli.com/collections/zigbee)
- [Home Assistant](https://www.home-assistant.io/)
- [Zigbee2MQTT Supported Devices](https://www.zigbee2mqtt.io/supported-devices/)
- [ratgdo](https://paulwieland.github.io/ratgdo/)
