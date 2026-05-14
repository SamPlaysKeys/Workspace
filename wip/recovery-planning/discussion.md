# Recovery Planning — Discussion

## 2026-05-11: Mapping Criticality

**Tier 0 (Air-Gap)**: MiniPC (cold)
- Purpose: Manual recovery if primary lab *and* cloud are unavailable
- Services: Forgejo (Git), Vault (secrets + CA), tsidp (identity)
- Update frequency: Weekly ZFS snapshots → encrypted USB handoff
- Recovery trigger: *Primary destroyed + oracle.net unreachable*

**Tier 1 (Warm Standby)**: Oracle Cloud VM
- Purpose: Ingress proxy for Tailscale clients + DNS failover
- Services: Tailscale relay node, CoreDNS, Komodo failover orchestrator
- Update frequency: Hourly R2 sync of critical configs
- Recovery trigger: *Primary lab down for >2h*

**Tier 2 (Hot Replication)**: Secondary homelab (future)
- Purpose: Full service mirror with live replication
- Services: OCP cluster + Docker hosts (identical to primary)
- Update frequency: Near-real-time Komodo sync
- Recovery trigger: *Primary degraded*

---

### Key Decisions Needed

1. **Critical service list**: Are Forgejo/Vault/miniflux top priority?
2. **DNS cutover**: Cloudflare → Tailscale SmartDNS or floating LB?
3. **Storage**: Can Oracle VM leverage R2 for "free" object storage?
4. **Orchestration**: Komodo → ArgoCD cutover for stateful services?