---
type: Idea
---

# Roadmap

Open questions, future work, and ideas for the homelab.

---

## Open Questions

These need to be resolved as the build progresses.

### Network Topology

- [x] Network segmentation between environments (VLANs) — defined in [homelab-map.md](../overview/homelab-map.md)
- [ ] Firewall rules between Prod/Test/Dev/IoT
- [ ] DNS strategy (internal DNS, split-horizon?)
- [ ] Reverse proxy / ingress strategy (Traefik, Caddy, nginx?)

### Storage Strategy

- [ ] UnRaid NAS role in Prod — just media, or also app data?
- [ ] Synology NAS role in Test — mirror of UnRaid structure, or different?
- [ ] Backup strategy across environments
- [ ] How does DevDocker access test data (if at all)?

### Terraform + Ansible Boundary

- [ ] Exactly which resources does Terraform manage?
- [ ] Does Terraform manage the physical MiniPCs, or just VMs on ProxMox?
- [ ] How do we bootstrap the Komodo Controller itself?

### Monitoring & Alerting

- [ ] Where does monitoring live? (Separate from workloads?)
- [ ] Komodo has built-in alerting — is that sufficient?
- [ ] Do we need a dedicated metrics/visualization stack (e.g. Prometheus/Grafana), or is Komodo + Uptime Kuma (+ existing logging) enough for the foreseeable future? **(Prometheus/Grafana/Influx-style stack explicitly deferred — not pursuing yet.)**

### Secrets Management

- [ ] How do secrets get into Komodo configs?
- [ ] Komodo supports variables — are those sufficient for secrets?
- [ ] Do we need external secrets management (Vault, etc.)?

---

## Planned Workloads

Apps and stacks to deploy once infrastructure is ready.

### Prod

| Workload | Type | Priority |
|----------|------|----------|
| Plex | Stack | High |
| *arr suite (Sonarr, Radarr, Prowlarr, etc.) | Stack | High |
| Uptime Kuma | App | Medium |
| *TBD* | — | — |

### Test

Mirror of Prod where validation is needed.

### Dev

Experimental containers, new apps being evaluated.

---

## Future Ideas

Things to consider but not immediately necessary.

### Multi-repo Strategy

Currently planning a single `infrastructure/` repo. Could split into:
- `infra-terraform/` — Just Terraform
- `infra-ansible/` — Just Ansible
- `homelab-apps/` — Just Komodo configs

**Tradeoff:** More repos = more isolation but more complexity. Single repo is simpler to start.

### CI/CD for Infra Repo

- Lint Terraform/Ansible/TOML on PR
- Plan Terraform changes on PR, apply on merge
- Validate Komodo TOML syntax before merge

### Automated Testing

- Spin up ephemeral Test environment for PRs
- Run smoke tests against deployed containers
- Tear down after validation

### Disaster Recovery

- Document recovery procedures for each environment
- Automate rebuild from scratch (Terraform + Ansible + Komodo)
- Backup strategy for stateful data (Plex metadata, *arr databases, etc.)

### Hardware Upgrades

- Upgrade path for MiniPCs if more compute needed
- GPU passthrough for Plex transcoding (currently planned for Prod MiniPC?)
- Additional storage nodes

---

## Decisions Still Pending

| Decision | Options | Notes |
|----------|---------|-------|
| Reverse proxy | Traefik, Caddy, nginx | Need to pick one |
| DNS | Pi-hole, AdGuard, external | Tied to network topology |
| Monitoring stack | Komodo + Uptime Kuma + log paths; metrics stack (Prometheus/Grafana, etc.) deferred | Env/power metrics also deferred |
| Secrets | Komodo variables, Vault, SOPS | Depends on sensitivity |
