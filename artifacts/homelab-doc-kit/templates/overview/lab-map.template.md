---
type: Reference
status: template
lab: {{LAB_NAME}}
---

# Lab Map — {{LAB_NAME}}

> Starter template. Fill this in during Phase 1 (aspirational) and refine in Phase 3 (real). This is the visual anchor for the whole documentation set.

## Environments

| Environment | Purpose | Subnet | Hardware & Services |
|-------------|---------|--------|---------------------|
| **Prod**    | Production services | 10.0.1.0/24 | _fill in_ |
| **Test**    | Staging / validation | 10.0.2.0/24 | _fill in_ |
| **Dev**     | Development, experiments | 10.0.3.0/24 | _fill in_ |
| **IoT**     | Smart home, cameras | 10.0.4.0/24 | _fill in_ |
| **User**    | Personal workstation | 10.0.10.0/24 | _fill in_ |

_Adjust the environment list to match your lab. This is a starting suggestion, not a mandate._

---

## Network Topology (sketch)

```
                         ┌──────────────────────────┐
                         │   Internet / ISP          │
                         └────────────┬─────────────┘
                                      │
                         ┌────────────▼─────────────┐
                         │   Router / firewall       │
                         └────────────┬─────────────┘
                                      │
        ┌──────────────┬──────────────┼──────────────┬──────────────┐
        ▼              ▼              ▼              ▼              ▼
   [ Prod VLAN ]  [ Test VLAN ]  [ Dev VLAN ]  [ IoT VLAN ]  [ User VLAN ]
        │              │              │              │              │
   _service_      _service_      _service_      _device_       _workstation_
```

_Fill in real device names, subnets, and service names as they're known. In Phase 1 this is aspirational; in Phase 3 it should reflect reality._

---

## Services & Dependencies

_List each service, the environment it lives in, and what it depends on._

| Service | Environment | Solves | Depends on |
|---------|-------------|--------|------------|
| _example_ | Prod | _problem it solves_ | _database, network_ |

---

## Visualization notes

- In Phase 1/2, this map is the **dream** — no IPs required, just intent.
- In Phase 3, add real subnets, hardware, and dependency edges.
- In Phase 4 (Fit), render this side-by-side with the Phase 1 version and color each item green/amber/red by fit. That overlay is the growth view.
