# Recovery Tiers

## Tier 0: Air-Gap (MiniPC @ Relative)
**Location**: Relative's home (same city)
**Hardware**: MiniPC (Intel NUC equivalent), ZFS mirror (2x 512GB NVMe)
**Services**:
- Forgejo (self-contained Git)
- Vault (disaster-recovery CA + secrets)
- tsidp (offline identity provider)

**Recovery Workflow**:
1. Insert USB → decrypt ZFS snapshot → restore services
2. Tailscale up → connect VPN clients
3. Forgejo/Vault DBs restored from last weekly snapshot

**Triggers**:
- Primary lab *destroyed*
- Oracle Cloud unreachable >48h

---

## Tier 1: Warm Standby (Oracle Cloud)
**Location**: ARM VM (us-phoenix-1), 1GB RAM, 100GB storage
**Services**:
- Tailscale relay node (failover VPN endpoint)
- CoreDNS (vanity domain over MagicDNS)
- Komodo failover orchestrator (pulls stateless configs from R2)
- miniflux (syncs from primary SQlite DB hourly)

**Traffic Flow**:
Client → Cloudflare  → Oracle DNS → Tailscale relay → Recovery endpoint
                     ↓
            WAF rules to drop malicious traffic

**Triggers**:
- Primary lab unreachable >2h
- Load balancer probes failing >3 consecutive checks

---

## Tier 2: Hot Replication (Future)
**Location**: Secondary homelab (cross-state)
**Services Mirrored**:
- Full OCP cluster (LenovoMini 3–5 replicas)
- Docker hosts (LenovoMini 1–2 replicas)
- JONSBO NAS (async replication via Syncthing)

**Sync Frequency**:
- Komodo: real-time pull-based
- Stateful DBs: hourly Rsync + WAL shipping

**Failover Workflow**:
1. Alert storm detected on primary
2. Komodo orchestrator on Oracle Cloud promotes DR cluster → updates DNS
3. Clients fail over to DR via Tailscale
