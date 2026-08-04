---
type: Note
layout: page
title: Home Assistant Automations
category: Homelab
status: Active
---


# Home Assistant Automations

Workflow definitions for IoT automations. Copy/adapt these for Home Assistant.

---

## Door Sensor Automations

### Door Left Open Alert

Announces via Google Home when a door has been open for 5 minutes.

```yaml
alias: "Alert - Door Left Open"
description: "Announce when a door has been open too long"
trigger:
  - platform: state
    entity_id:
      - binary_sensor.front_door
      - binary_sensor.back_door
      - binary_sensor.office_door
      - binary_sensor.garage_entry_door
    to: "on"
    for:
      minutes: 5
action:
  - service: tts.google_translate_say
    target:
      entity_id: media_player.living_room_speaker
    data:
      message: "{{ trigger.to_state.attributes.friendly_name }} has been open for 5 minutes."
mode: parallel
```

---

### Door Still Open Reminder

Repeats announcement every 5 minutes while an exterior door remains open.

```yaml
alias: "Alert - Door Still Open Reminder"
description: "Repeat reminder every 5 minutes while door is open"
trigger:
  - platform: state
    entity_id:
      - binary_sensor.front_door
      - binary_sensor.back_door
    to: "on"
    for:
      minutes: 5
action:
  - repeat:
      while:
        - condition: state
          entity_id: "{{ trigger.entity_id }}"
          state: "on"
      sequence:
        - service: tts.google_translate_say
          target:
            entity_id: media_player.living_room_speaker
          data:
            message: "Reminder: {{ trigger.to_state.attributes.friendly_name }} is still open."
        - delay:
            minutes: 5
mode: parallel
```

---

### Garage Entry Night Alert

Alerts when garage entry door opens at night, with camera snapshot.

```yaml
alias: "Alert - Garage Entry Night"
description: "Alert when garage entry opens at night"
trigger:
  - platform: state
    entity_id: binary_sensor.garage_entry_door
    to: "on"
condition:
  - condition: time
    after: "22:00:00"
    before: "06:00:00"
action:
  - service: tts.google_translate_say
    target:
      entity_id: media_player.bedroom_speaker
    data:
      message: "Garage entry door opened."
  - service: camera.snapshot
    target:
      entity_id: camera.tapo_garage
    data:
      filename: "/config/www/snapshots/garage_{{ now().strftime('%Y%m%d_%H%M%S') }}.jpg"
mode: single
```

---

## Lighting Automations

### Garage Light Button

Wireless button in adjacent room toggles garage light.

```yaml
alias: "Button - Garage Light Toggle"
description: "Wireless button controls garage light"
trigger:
  - platform: event
    event_type: zha_event
    event_data:
      device_id: <zigbee_button_device_id>
      command: "toggle"
action:
  - service: light.toggle
    target:
      entity_id: light.garage
mode: single
```

**Note:** Can also use Zigbee direct binding for button-to-switch control without HA in the loop.

---

### Goodnight Scene

Single button or voice command triggers bedtime routine.

```yaml
alias: "Scene - Goodnight"
description: "Turn off lights, set fan, dim bedroom"
trigger:
  - platform: event
    event_type: zha_event
    event_data:
      device_id: <bedside_button_device_id>
      command: "on"
action:
  - service: light.turn_off
    target:
      entity_id:
        - light.hall_light
        - light.garage
  - service: light.turn_on
    target:
      entity_id: light.bedroom_fan_light
    data:
      brightness_pct: 10
  - service: fan.set_percentage
    target:
      entity_id: fan.bedroom_fan
    data:
      percentage: 50
mode: single
```

---

## Presence Automations

### Arriving Home

Turns on entry lights when arriving home after sunset.

```yaml
alias: "Presence - Arriving Home"
description: "Turn on lights when someone arrives after sunset"
trigger:
  - platform: state
    entity_id: person.you
    to: "home"
condition:
  - condition: sun
    after: sunset
action:
  - service: light.turn_on
    target:
      entity_id:
        - light.front_porch
        - light.living_room
mode: single
```

**Presence detection options:**
- HA Companion app (phone GPS)
- Unifi device tracker (via Unifi integration)
- Bluetooth/BLE presence sensors (future)

---

## Climate Automations (Future)

With Sensi thermostats integrated, potential automations:

### Away Mode

```yaml
alias: "Climate - Away Mode"
description: "Set thermostats to away when everyone leaves"
trigger:
  - platform: state
    entity_id: group.all_people
    to: "not_home"
    for:
      minutes: 30
action:
  - service: climate.set_preset_mode
    target:
      entity_id:
        - climate.sensi_upstairs
        - climate.sensi_downstairs
    data:
      preset_mode: "away"
mode: single
```

### Pre-condition Before Arrival

```yaml
alias: "Climate - Pre-condition"
description: "Resume normal temps when someone is heading home"
trigger:
  - platform: zone
    entity_id: person.you
    zone: zone.near_home
    event: enter
action:
  - service: climate.set_preset_mode
    target:
      entity_id:
        - climate.sensi_upstairs
        - climate.sensi_downstairs
    data:
      preset_mode: "home"
mode: single
```

---

## Integration Notes

### Google Cast TTS

Uses `tts.google_translate_say` service. Requires:
- Google Cast integration configured in HA
- Speaker entity IDs (e.g., `media_player.living_room_speaker`)

### Tapo Camera Snapshots

Requires Tapo integration or generic camera with RTSP. Snapshots save to `/config/www/` for web access.

### Zigbee Events

Button events use `zha_event` (for ZHA) or `mqtt` triggers (for Zigbee2MQTT). Device IDs and commands vary by button model — check HA device page after pairing.

---

## Receipt Automation

### Receipt Fridge Launch — NFC App Launch

Tap the fridge NFC tag to open the receipt scanner app directly.

```yaml
alias: "Receipts - NFC App Launch"
description: "Launch receipt scanner app from fridge NFC"
trigger:
  - platform: nfc
    tag_id: <fridge_receipt_nfc_uid>
action:
  - service: notify.mobile_app_<phone>
    data:
      message: "open_receipt_scanner"
      data:
        action: launch_app
        package: com.example.receiptscanner
mode: single
```

The NFC tag is the entry point. The app handles the actual capture and any per-receipt workflow.

**Considerations:**
- If the app supports deep links or URL schemes, prefer that over a generic notification.
- If people use different phones/scanner apps, map multiple `notify` targets or a `choose` block by person/presence.

---

## Service Check / Router Status NFC

### Router Status — NFC Status Page

Opens a locally hosted status page showing service health.

```yaml
alias: "Status - Router NFC"
description: "Serve local status page for router/service health"
trigger:
  - platform: nfc
    tag_id: <router_status_nfc_uid>
action:
  - service: browser_mod.popup
    data:
      title: "Service Status"
      # Serve from a local/static endpoint; no external CDN deps
      url: "/local/status/index.html"
mode: single
```

**Implementation notes:**
- Host `status/index.html` on the same host that serves HA or a lightweight internal web server.
- Keep the page plain HTML with no external dependencies so it works when the internet is down.
- Display gateway ping, DNS resolution result, Pi-hole stats, and a one-click router reboot if supported.

---

## Consumable Replacement NFC

### Consumable Tag — Reorder Trigger

Opens the replacement item's reorder path.

```yaml
alias: "Consumables - Replace"
description: "Open consumable replacement action"
trigger:
  - platform: nfc
    tag_id: <consumable_nfc_uid>
action:
  - service: browser_mod.popup
    data:
      title: "Replace Air Filter"
      # Prefer a stable local/HA endpoint; the destination can rotate behind this URL
      url: "/local/consumables/air-filter.html"
mode: single
```

Instead of encoding a retailer URL directly, use a stable local or Home Assistant destination so the reorder page can change without rewriting the tag.

---

## Pi-hole Temporary Disable NFC

### Pi-hole — 30 Minute Disable

Disables Pi-hole time-boxed for 30 minutes.

```yaml
alias: "Pi-hole - Temp Disable 30m"
description: "Disable Pi-hole for 30 minutes from NFC tap"
trigger:
  - platform: nfc
    tag_id: <pihole_temp_disable_nfc_uid>
action:
  - service: rest_command.pihole_disable_30m
mode: single
```

Time-box the disable window so ad-blocking is re-enabled automatically. Surface the remaining time on the local status page so the household can see the countdown.
