---
type: Troubleshooting
status: Active
system: Tailscale
related_to:
  - tailscale
  - networking
references:
  - https://tailscale.com/kb/1241/app-connectors
---

# Tailscale `--accept-routes` Overlapping with Local Subnet

## Symptoms

- Other machines on the same LAN/subnet cannot reach the VM (ping fails, services unreachable).
- The VM cannot reach other machines on the same subnet, including the gateway.
- External connectivity still works through Tailscale (SSH via Tailscale IP succeeds).
- `ping -I <local_interface> <gateway>` works, but `ping <gateway>` without interface flag does not.

## Root Cause

Tailscale's `--accept-routes` flag is enabled on the machine. When another Tailscale node (or an App Connector) advertises a subnet route that overlaps with the machine's local subnet (e.g., `10.0.1.0/24`), Tailscale injects that route into routing **table 52** (checked before the main routing table). This causes all traffic to the local subnet to be routed through `tailscale0` instead of the physical interface (`ens18`, `eth0`, etc.), breaking local network communication.

## Resolution

Disable route acceptance on the machine:

```bash
tailscale set --accept-routes=false
```

This removes the overlapping route from table 52 and restores normal local subnet routing.

## Verification

Confirm the route is gone and local connectivity is restored:

```bash
# Verify no overlapping route in table 52
ip route show table 52 | grep <your_subnet>

# Test local subnet connectivity
ping -c 2 <gateway>
ping -c 2 <local_peer>

# Verify other machines can reach this machine
# (Run from another machine on the same subnet)
ping -c 2 <this_machine_ip>
```

## References

- [Tailscale App Connectors](https://tailscale.com/kb/1241/app-connectors) - Routes can be advertised via App Connectors on the tailnet, which get accepted when `--accept-routes` is enabled.
