# Running Discussion

## Session Start

Reviewing four prepared notes: agent_context, project_scope, solution_draft, upgrade_strategy_summary.

Current state: foundation is laid — problem definition, strategy direction, in/out of scope, 5-phase plan, policy layers table, architectural concept (Ansible + KubeVirt native controls), and upgrade workflow diagrams.

## 2026-07-15
Established a detailed upgrade orchestration decision tree in `README.md` to classify migration and node upgrade workflows depending on infrastructure tooling context:
1. **Ansible Core (Engine-only)**: Runbook-driven CLI execution interacting directly with OpenShift / KubeVirt APIs.
2. **Ansible Automation Platform (AAP)**: Centralized Job/Workflow templates enabling audit logging, secure credentials, and GitOps-ish webhook integration.
3. **AAP + Red Hat Advanced Cluster Management (ACM)**: Multi-cluster enterprise pattern combining declarative governance policies with AAP automation to coordinate pre-migration, node maintenance, and post-migration validation at scale.

### Execution Guide Development
Created comprehensive execution guides under `execution-guides/` with three parallel tracks:

**Structure per method:**
- `design.md` — Narrative architecture, component interactions, failure modes, rollback strategy
- `checklist.md` — Step-by-step execution guide with verification gates
- `reference.md` — CRD schemas, API references, external documentation links

**Completed:**
- `execution-guides/ansible-core/` — Runbook-driven CLI path (lighter treatment)
- `execution-guides/aap/` — Centralized orchestration with GitOps-ish triggers, webhook/API integration, RBAC
- `execution-guides/aap-acm/` — Fleet governance with ACM policies triggering AAP workflows, PlacementRules, AnsibleJob CRD

**Backlog additions:**
- AAP Controller installation & configuration documentation
- ACM Hub-to-managed-cluster topology documentation

### ADR: VM Policy Thresholds
Created `adr/0001-vm-policy-thresholds.md` defining classification thresholds for VM live migration:

**Memory thresholds:**
- Small: < 16 GB → standard pre-copy
- Medium: 16-64 GB → tuned pre-copy
- Large: 64-256 GB → auto-converge eligible
- Very Large: > 256 GB → post-copy or manual planning

**Dirty-rate thresholds:**
- Low: < 500 pages/sec → standard
- Moderate: 500-2000 pages/sec → extended timeout
- High: 2000-5000 pages/sec → auto-converge
- Extreme: > 5000 pages/sec → post-copy (approved)

Includes four policy profile definitions (baseline, enhanced, autoconverge, postcopy) with YAML CRs, trade-off analysis, and measurement methods.

### Migration Timeout Calculation Reference
Created `migration-timeout-calculation.md` explaining how `CompletionTimeoutPerGiB` is calculated:

**Key insight:** Value depends on network bandwidth and dirty-page iteration multiplier:
- Dedicated 10+ Gbps: 2-5 sec/GiB
- Dedicated 1-10 Gbps: 5-10 sec/GiB
- Shared/overcommitted: 20-50 sec/GiB

**Formula:** `Total Timeout = CompletionTimeoutPerGiB × VM Memory (GiB)`

Updated ADR to reference this doc and use environment-aware calibration instead of fixed values.


