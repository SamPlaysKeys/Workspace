# Recovery Environment Planning

**Goal**: Design a tiered recovery environment incorporating:
- **MiniPC at relative's house**: Air-gapped recovery island
- **Oracle Cloud VM**: Proxy/relay failover + traffic egress
- **Full cross-region replication**: Automated failover for critical services

**Current assets**:
- MiniPC (hw specs TBD; assume 4-core, 16GB RAM, 512GB SSD)
- Oracle Cloud VM: ARM-based, 1GB RAM, 100GB storage (expandable)
- Homelab: Unifi+Tailscale network, Komodo orchestration

**Open questions**:
- What services are "critical" for recovery? (candidate list: Forgejo, Vault, miniflux, tsidp, Komodo)
- How to segment recovery tiers (cold → warm → hot)?
- What are the trigger conditions for failover?
- How to handle DNS cutover (Cloudflare? Tailscale MagicDNS?)?

**Key constraints**:
- Physical distance: MiniPC 80 mi away, Oracle Cloud cross-region
- Bandwidth: Relative has 100 Mbps upload
- Security: Air-gapped must remain offline until disaster declared
- Cost: Oracle Cloud under free tier where possible