---
type: Troubleshooting
status: Active
system: Ubuntu VDI (Omnissa Horizon)
related_to: []
references: []
---

# Troubleshooting: Blank Screen or Unresponsive Session in Horizon Client

## Symptoms
- Successful login via Horizon client or web client.
- Immediate transition to a blank screen after authentication.
- Lack of response to mouse movements or keyboard input within the session.
- Window resizing/scaling is non-functional.

## Troubleshooting & Identification Steps

### 1. Agent Service Verification
Confirm the Horizon Agent service is active and running.
```bash
systemctl status viewagent.service
```

### 2. Network & Port Verification
Ensure the agent is listening on the expected ports (e.g., 443, 4172) and that no local firewall (like `ufw`) is blocking them.
```bash
ss -tunlp | grep -E 'viewagent|443|4172'
sudo ufw status
```

### 3. Session & Display Logs
Investigate session startup errors and potential display server (Xorg/Wayland) crashes.
```bash
journalctl --user -b 0 | grep -Ei 'session|gnome|xfce|wayland|xorg'
sudo grep -Ei 'error|failed|fatal' /var/log/Xorg.*
```

### 4. Resource Availability
Ensure the system has sufficient disk space and memory to launch the desktop environment.
```bash
df -h
free -h
top -bn1 | head -n 20
```

## Resolution Callout
> **Note:** If all identification steps confirm the services and resources are healthy, ensure your system is fully updated:
> `sudo apt update && sudo apt upgrade`
