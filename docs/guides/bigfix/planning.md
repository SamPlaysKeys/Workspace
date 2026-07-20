---
type: Guide
status: Active
system: HCL BigFix
related_to: []
references:
  - HCL BigFix Platform Documentation: https://help.hcl-software.com/bigfix/11.0/platform/index.html
---

# HCL BigFix Planning & Architecture Guide

## Overview

HCL BigFix is a multi-platform endpoint management platform that enables IT operations and security teams to discover, manage, patch, and secure global IT estates. Whether operating on-premises, in the cloud, or in hybrid environments, BigFix uses a unique single-agent, hierarchical architecture to manage thousands of devices efficiently with minimal bandwidth overhead.

This guide is designed for technical consultants planning a new BigFix deployment with a client. It outlines the core architectural components, infrastructure decisions, prerequisite checklists, deployment flows, and discovery questions necessary to align with a client's requirements.

---

## BigFix Core Architecture & Components

BigFix does not rely on active scanning or heavy database polling. Instead, it operates on a pull-based, "Relevance" evaluation model. 

```
                                +-------------------+
                                |  Windows Console  |
                                +---------+---------+
                                          |
                                          | (HTTPS - TCP 52311)
                                          v
+-------------------+           +-------------------+           +-------------------+
|    Web Reports    |<----------|    Root Server    |---------->|  HCL License /    |
+-------------------+  (TCP)    +---------+---------+  (HTTPS)  |  Content Servers  |
                                          |                     +-------------------+
                                          | (TCP/UDP 52311)
                                          v
                                +-------------------+
                                |    Top-Level      |
                                |      Relay        |
                                +---------+---------+
                                          |
                   +----------------------+----------------------+
                   | (TCP/UDP 52311)                             | (TCP/UDP 52311)
                   v                                             v
         +-------------------+                         +-------------------+
         |   Branch Relay    |                         |   Branch Relay    |
         |    (Location A)   |                         |    (Location B)   |
         +---------+---------+                         +---------+---------+
                   |                                             |
         +---------+---------+                         +---------+---------+
         |                   |                         |                   |
         v                   v                         v                   v
   +-----------+       +-----------+             +-----------+       +-----------+
   |  Client   |       |  Client   |             |  Client   |       |  Client   |
   +-----------+       +-----------+             +-----------+       +-----------+
```

### 1. The Root Server
The central brain of the deployment. It:
- Collects and aggregates reports from agents.
- Downloads patch feeds and fixlets from the HCL Content Servers.
- Manages and distributes action payloads.
- Runs the core database services.

### 2. BigFix Relays
Relays are intermediate staging points. Instead of 10,000 agents downloading a 100MB patch over a WAN link from the Root Server, the local Relay downloads it once, and the local agents fetch it from that Relay over the local LAN.
- **Relays are standard endpoints** running a lightweight relay service.
- **Hierarchical layering:** Relays can report to other parent relays (Top-Level Relays), shielding the Root Server from direct load.

### 3. BigFix Client (Agent)
A lightweight agent (~10-20MB RAM, <2% CPU) running as a system service on every managed machine.
- It continuously evaluates the endpoint state using **Relevance Language** (a declarative query language).
- If an agent determines a patch or configuration is "relevant", it flags itself in the Console.
- It executes **Action Scripts** when instructed by the operator.

### 4. BigFix Console
The primary administrative interface used by operators to view inventory, author actions, and deploy patches.
- **Constraint:** The Console is strictly a **Windows-only** desktop application.
- It connects directly to the Root Server via HTTP/HTTPS.

### 5. Web Reports
A web-based interface for secondary reporting, viewing historical trends, and sharing read-only compliance dashboards with executive stakeholders.

---

## Infrastructure Sizing & Database Choices

Selecting the right OS and Database backend is the most critical decision during the initial phase.

### Supported Operating Systems for Root Server
- **Windows Server:** 2016, 2019, 2022 (Standard and Datacenter editions).
- **Red Hat Enterprise Linux (RHEL):** 8.x, 9.x (64-bit).

### Database Backends & Constraints
- **Microsoft SQL Server (for Windows Server deployments):**
  - **SQL Server Enterprise & Standard:** Supported for versions 2014 through 2022.
  - **SQL Server Express:** Supported *only* for proof-of-concept (POC) environments or environments with **fewer than 500 endpoints** due to the 10GB database size limit.
- **IBM DB2 (for Linux deployments):**
  - DB2 v11.5 is the standard for Linux deployments.
  - An evaluation/bundled version of DB2 is typically included in the Linux server installer.

### Hardware Sizing Guidelines
*Note: Sizing depends on endpoint counts and active modules (e.g., Lifecycle, Compliance, Inventory).*

| Endpoint Count | Role | CPU Cores | RAM | Storage Type | Recommended Disk IOPS |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **< 1,000** | All-in-One Server | 4 Cores | 16 GB | SSD (SAS/SATA) | 500+ |
| **1,000 - 10,000** | Root Server + DB | 8 Cores | 32 GB | High-Speed NVMe/SSD | 2,000+ |
| **10,000 - 50,000** | Split Root & DB Server | 16 Cores | 64 GB+ | Dedicated Enterprise SAN | 5,000+ |

---

## Prerequisites

### 1. Network & Firewall Port Requirements
The BigFix platform uses **TCP/UDP port 52311** as its default communication channel.

| Source | Destination | Port / Protocol | Direction | Purpose |
| :--- | :--- | :--- | :--- | :--- |
| Windows Console | Root Server | TCP 52311 | Inbound | Console-to-Server connection |
| Root Server | Internet (HCL) | TCP 80 / 443 | Outbound | Gathering sync feeds and downloads |
| Relays | Root Server | TCP 52311 | Inbound | Reporting parent-upstream status |
| Clients (Agents) | Local Relay | TCP 52311 | Inbound | Client report uploads & payloads |
| Relays / Server | Clients (Agents) | UDP 52311 | Outbound | "Ping" notification to trigger fast checks |
| Web Reports Server | Root Server / DB | TCP 1433 or 50000 | Inbound | SQL Server (1433) or DB2 (50000) access |

*Note on UDP 52311:* If UDP is blocked across subnets, agents will still check in via TCP on a polling interval (defaulting to every 4 hours), but real-time "gather now" command execution will be delayed.

### 2. DNS & Name Resolution
- The Root Server must be reachable via a fully qualified domain name (FQDN) or a dedicated IP.
- It is highly recommended to use a **CNAME alias** (e.g., `bigfix.clientdomain.com`) during installation rather than a specific server host name. This allows easy migration of the hardware/VM in the future without redeploying agents.

---

## Configuration & Installation Sequence

The general deployment path follows these distinct phases:

### Phase 1: License Authorization & Masthead Creation
1. Run the **BigFix Installation Generator** on a Windows staging machine.
2. Submit your HCL license authorization file (`.BesLicenseRequest`) to the HCL License server.
3. HCL returns a Signed License file.
4. Generate the **Masthead file (`actionsite.afxm`)**. This file acts as the anchor of trust. It contains the server FQDN, configuration parameters, and the public key used by agents to verify that actions sent to them are authentically signed.

### Phase 2: Installing the Root Server
- **On Windows:** Run `setup.exe` and choose "Install Server". Configure your connection to MS SQL Server and select your generated masthead.
- **On Linux:** Extract the tarball, ensure database (DB2/external) is ready, and run:
  ```bash
  ./install.sh
  ```
  Select Option `[1] All Components` or `[2] Server and Client Only` when prompted.

### Phase 3: Deploying the Console
1. Run the console installer on a Windows administrator workstation.
2. Connect to the Root Server using the operator account created during server installation.

### Phase 4: Deploying Relays
1. Install the standard BigFix Agent on dedicated relay VMs or branch office hosts.
2. In the BigFix Console, right-click the designated computers and select **"Install BigFix Relay"**.
3. Configure the relay settings (e.g., maximum cache size, parent relays).

### Phase 5: Agent Rollout
- Use the **Client Deploy Tool (CDT)** built into the Console to push agents over SSH or WMI.
- For large scale deployments, deploy the agent MSI/RPM package via GPO, SCCM, or Ansible, bundled with the generated `actionsite.afxm` masthead.

---

## Consultant's Discovery Checklist

Use these specific discovery questions during your initial meeting with the client to gather architectural requirements:

1. **How many total endpoints (workstations, servers, VMs, containers) are in scope?**
   * *Why:* Determines whether SQL Server Express is acceptable (< 500) and sets hardware specifications.
2. **What is the current distribution of Operating Systems across these endpoints?**
   * *Why:* Identifies if they need specific OS agents and helps plan the deployment method.
3. **What is the network topology? Do you have remote branch offices or bandwidth-constrained links?**
   * *Why:* Drives the relay placement strategy and WAN bandwidth throttle planning.
4. **Do you have a preferred Operating System for the central BigFix server (Windows Server vs RHEL)?**
   * *Why:* Dictates database licensing requirements (MS SQL Server vs DB2) and administrative skillsets.
5. **How are endpoints currently patched or managed? Do we need to plan co-existence or a migration from tools like WSUS, SCCM, or Ansible?**
   * *Why:* Highlights potential software conflicts or open firewall port struggles.
6. **Will endpoints reporting from outside the corporate firewall (remote/WFH workers) need management?**
   * *Why:* Determines if we need to deploy an **Internet-Facing Relay (DMZ Relay)** with public DNS records.

---

## Common Pitfalls & Anti-Patterns

- **Hardcoding Server Names:** Installing with a specific server name (e.g., `srv-bf-prod-01`) instead of a DNS CNAME alias (`bigfix.company.com`). If the server hardware dies, migrating the system requires manually updating the masthead on every single agent.
- **Neglecting Relay Cache Limits:** Leaving relay cache limits at default values. In environments deploying heavy OS patches or major software rollouts, relays will run out of cache space, causing them to fetch payloads repeatedly across WAN links.
- **Ignoring UDP 52311:** Assuming TCP is sufficient. Without UDP 52311 inbound to clients, operators cannot trigger "Instant" actions, severely impacting incident response speeds.
- **Insufficient Space in Temporary Directories:** Installing without verifying disk space in `/var/tmp` (Linux) or `C:\Windows\Temp` (Windows). BigFix client downloads are staged here, and major patch actions will fail if these directories are constrained.

---

## References

- [HCL BigFix Enterprise Support Portal](https://support.hcl-software.com/csm) - Central knowledge base and product updates.
- [BigFix Developer Reference](https://developer.bigfix.com/) - Authoring guides, Relevance query guides, and REST API references.
