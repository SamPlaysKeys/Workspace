---
type: Note
related_to:
- '[[tailscale]]'
status: Active
layout: page
title: Unifi Zone-Based Firewall (ZBF) Guide
category: Homelab
---


# Unifi Zone-Based Firewall (ZBF) Guide

## Overview
This guide walks through configuring the homelab network using UniFi's Zone-Based Firewalls (ZBF), available in UniFi Network 9.0+. 

ZBF shifts away from traditional LAN In/Out rules by grouping networks into **Zones** and defining **Policies** that dictate traffic flow between those zones. This is visualized in the Zone Matrix and is vastly superior for environment segmentation.

## 1. Define Custom Zones
By default, UniFi places all local networks into the built-in `Internal` zone. To segment our environments, we will create custom zones that perfectly map to our VLANs.

Go to **Settings > Security > Firewall > Zones** and create the following custom zones:

| Zone Name | Assigned Network (VLAN) | Purpose |
|-----------|--------------------------|---------|
| **Prod**  | Prod (1)                 | Core services, Home Assistant |
| **Test**  | Test (2)                 | Pre-production validation |
| **Dev**   | Dev (3)                  | OCP and Sandbox nodes |
| **IoT**   | IoT (4)                  | Cameras, smart plugs |
| **DMZ**   | DMZ (5)                  | Public-facing services (qBittorrent) |
| **User**  | User (10)                | Laptops, phones, trusted devices |

*(Note: Ensure each VLAN is moved out of the default `Internal` zone and into its respective custom zone.)*

### Built-in Zones
Alongside our custom zones, the following built-in UniFi zones remain actively used:
- **Hotspot:** Contains the `Guest` network (VLAN 20). UniFi automatically isolates this zone (blocks access to all other zones) while allowing internet access, perfectly mapping to guest network needs.
- **Gateway:** Represents the router itself. Essential for all zones to access DHCP, DNS, and Gateway functions.
- **External:** Represents the Internet. 

## 2. Configure the Zone Matrix (Policies)

Instead of a linear list of "Drop" rules, ZBF uses a matrix (source zone on the Y-axis, destination zone on the X-axis). We define explicit policies for allowed paths and lock down the rest.

Go to **Settings > Security > Firewall > Policy Table** (or click the cells in the Zone Matrix) and configure the following policies:

### Policy 1: HA Control over IoT
*Allows Home Assistant to reach and control IoT devices.*
- **Name:** Allow HA to IoT
- **Action:** Allow
- **Source Zone:** Prod
- **Destination Zone:** IoT
- **Source Device/IP:** `10.0.1.X` (Home Assistant IP)
- **Auto Allow Return Traffic:** Checked (Ensures IoT devices can respond to HA)

### Policy 2: User Local Access to Prod (Optional for Latency)
*Allows User VLAN devices fast local access to specific Prod services (bypassing Tailscale).*
- **Name:** Allow User to Prod Apps
- **Action:** Allow
- **Source Zone:** User
- **Destination Zone:** Prod
- **Destination Port:** Select specific ports (e.g., `8123` for HA, `32400` for Plex)
- **Auto Allow Return Traffic:** Checked 

### Policy 3: Allow User to IoT (Chromecast/Casting)
*Allows phones/laptops on User VLAN to cast media to Chromecasts and smart speakers on IoT VLAN.*
- **Name:** Allow User to IoT Casting
- **Action:** Allow
- **Source Zone:** User
- **Destination Zone:** IoT
- **Destination Port:** `8008`, `8009` (TCP - Chromecast control ports)
- **Auto Allow Return Traffic:** Checked

### Policy 4: Prod to Test Synchronization
*Allows Prod services to sync with Test.*
- **Name:** Allow Prod to Test
- **Action:** Allow
- **Source Zone:** Prod
- **Destination Zone:** Test
- **Auto Allow Return Traffic:** Checked

### Policy 5: Test to Dev Validation
*Allows Test services to reach Dev.*
- **Name:** Allow Test to Dev
- **Action:** Allow
- **Source Zone:** Test
- **Destination Zone:** Dev
- **Auto Allow Return Traffic:** Checked

### Policy 6: DMZ to External (Internet)
*Allows DMZ services (e.g. qBittorrent) to reach the internet for torrent traffic and updates.*
- **Name:** Allow DMZ to External
- **Action:** Allow
- **Source Zone:** DMZ
- **Destination Zone:** External
- **Auto Allow Return Traffic:** Checked

### Policy 7: External to DMZ (Specific Ports)
*Allows incoming connections to DMZ services on required ports. Tightly scoped to only what's needed.*
- **Name:** Allow External to DMZ Services
- **Action:** Allow
- **Source Zone:** External
- **Destination Zone:** DMZ
- **Destination Port:** `6881` (TCP — torrent); `6881` (UDP — DHT); add more as new services are added
- **Auto Allow Return Traffic:** Checked
- **Notes:** This is the only inbound path from the internet into the lab. Review and restrict per-service.

### Policy 8: DMZ to UnRaid NAS (Shared Storage)
*Allows DMZ services to write downloaded content to the UnRaid NAS NFS export so Prod services can access it.*
- **Name:** Allow DMZ to UnRaid NFS
- **Action:** Allow
- **Source Zone:** DMZ
- **Destination Zone:** Prod
- **Source Device/IP:** DMZ subnet (10.0.5.0/24)
- **Destination IP:** UnRaid NAS IP (10.0.1.X)
- **Destination Port:** `2049` (TCP — NFS)
- **Auto Allow Return Traffic:** Checked
- **Notes:** This is the **only** exception to DMZ → Internal block. Tightly scoped to a single IP + port.

## 3. Enable Multicast DNS (mDNS) for Discovery

For casting to work across VLANs, devices on the `User` VLAN must be able to discover the Chromecasts on the `IoT` VLAN. UniFi handles this via a built-in mDNS reflector.

Go to **Settings > Networks > Global Network Settings** (or the specific network settings) and ensure **Multicast DNS** is enabled for the `User` and `IoT` networks.

*(Note: The UniFi Gateway automatically permits mDNS discovery traffic under the hood when this feature is turned on, so no explicit ZBF policy is needed for the discovery phase—only for the streaming connection, which Policy 3 handles).*

## 4. Enforce Default Deny Between Custom Zones

To complete the segmentation, we must ensure that all other inter-zone traffic is blocked. UniFi's matrix allows you to set the baseline interaction between zones.

For any zone pair that shouldn't communicate (e.g., `IoT` to `User`, `User` to `Dev`, `DMZ` to any internal zone), create a catch-all block policy:

- **Name:** Block Inter-Zone Traffic
- **Action:** Block
- **Source Zone:** Select all custom zones (`Prod, Test, Dev, IoT, User, DMZ`)
- **Destination Zone:** Select all custom zones (`Prod, Test, Dev, IoT, User`)
- *Note: Place this policy at the very bottom of the Policy Table so our explicit "Allow" rules above take precedence. (The built-in `Hotspot` zone is natively blocked from other zones by UniFi, so it does not need to be included in this block list).*

## 5. Port Forwarding — qBittorrent

Port forwarding is required for qBittorrent to accept incoming peer connections, which improves swarm connectivity and satisfies tracker connectability requirements.

### Configure Port Forward Rule

Go to **Settings > Security > Traffic Rules > Port Forwarding** and create:

| Field | Value | Notes |
|-------|-------|-------|
| **Name** | qBittorrent Incoming | |
| **Enabled** | True | |
| **From** | Any | Accepts connections from any WAN IP |
| **Port** | `51820` | Pick a high ephemeral port (avoid `6881` — some ISPs throttle well-known BitTorrent ports) |
| **Forward IP** | `10.0.5.X` | DMZ machine's static IP running qBittorrent |
| **Forward Port** | `51820` | Can differ from WAN port if needed, but matching is cleaner |
| **Protocol** | TCP & UDP | Both needed: TCP for peer data, UDP for DHT/magnet links |
| **Log** | False | Avoid log noise |

### qBittorrent Configuration

In qBittorrent, set the incoming port to match the forward port:

**Settings > Connection > Listening Port:** `51820`

### Relationship to ZBF Policies

- The port forward rule implicitly creates a firewall exception on the WAN interface.
- ZBF Policy 7 (External → DMZ) still governs east-west traffic once the packet arrives at the DMZ VLAN.
- If Policy 7's destination port is locked to `6881`, you must update it to match your chosen forward port (`51820`), or widen it to cover both.

### Optional: Static DHCP Lease

Ensure the DMZ machine's IP does not change:

**Settings > Networks > LAN (DMZ VLAN) > DHCP > DHCP Lease Table:**
- Add a fixed lease for the qBittorrent MAC → `10.0.5.X`

## 6. Internet Access (External Zone)

The `External` zone represents the Internet. 

- **User, Prod, Test, Dev to External:** Generally allowed by default so containers can pull images and devices can update.
- **DMZ to External:** Allowed via Policy 6.
- **IoT to External:** 
  - To block IoT devices from "phoning home", create a policy: **Block IoT to Internet**.
  - **Source Zone:** IoT
  - **Destination Zone:** External
  - **Action:** Block
  - *(Optional: Create a higher-priority Allow rule for specific IoT devices that require cloud connectivity, like a smart vacuum).*