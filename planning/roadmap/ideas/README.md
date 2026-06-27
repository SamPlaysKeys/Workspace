# Ideas

Future possibilities and brainstorms that haven't made it to the roadmap yet.

This is a holding area for ideas that might become features, behaviors, or projects. Not everything here will be implemented — it's for capturing thoughts before they're lost.

## Receipt NFC App Launch

**Status:** Idea  
**Area:** Home automation / IoT  
**Trigger:** NFC tag on fridge.  
**Action:** Phone reads tag → launches receipt scanner app directly, eliminating drop-zone friction.

The tag is the entry point, not a drop zone. Open the app to ready state; actual capture happens inside the app.

See `docs/homelab/iot/automations.md` for automation flows.

## Router Status NFC

**Status:** Idea  
**Area:** Home automation / IoT  
**Trigger:** NFC tag near router.  
**Action:** Opens a locally hosted offline-safe status page showing service health, DNS/gateway status, and quick actions.

Use case: when something feels off, tap the tag to verify whether the network or a downstream service is down.

Design constraint: keep the status page static and fully local with no external CDN dependencies.

See `docs/homelab/iot/automations.md` for implementation notes.

## Consumable Replacement NFC

**Status:** Idea  
**Area:** Home automation / IoT  
**Trigger:** NFC tag attached to an appliance or consumable storage location.  
**Action:** Opens the replacement item's detail or reorder path.

Example: air filters. The tag resolves to Home Assistant, a product page, or a rest_command-backed reorder flow so the target can change without rewriting the tag.

See `docs/homelab/iot/automations.md` for implementation notes.

## Pi-hole Temporary Disable NFC

**Status:** Idea  
**Area:** Home automation / IoT  
**Trigger:** NFC tag in a convenient location.  
**Action:** Disables Pi-hole/AdGuard for 30 minutes, with visible countdown on the status page.

This should be time-boxed, not an indefinite toggle, to avoid household-wide ad leakage from forgotten toggles.

See `docs/homelab/iot/automations.md` for automation rules.
