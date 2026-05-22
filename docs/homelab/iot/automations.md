---
type: Note
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
