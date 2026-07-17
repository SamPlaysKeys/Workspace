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

### Migration Timeout Calculation Reference
Created `migration-timeout-calculation.md` explaining how `CompletionTimeoutPerGiB` is calculated:

**Key insight:** Value depends on network bandwidth and dirty-page iteration multiplier:
- Dedicated 10+ Gbps: 2-5 sec/GiB
- Dedicated 1-10 Gbps: 5-10 sec/GiB
- Shared/overcommitted: 20-50 sec/GiB

**Formula:** `Total Timeout = CompletionTimeoutPerGiB × VM Memory (GiB)`

### Artifacts Directory Creation
Created `artifacts/` with skeleton examples for implementation:

**Structure:**
```
artifacts/
├── playbooks/           # 4 Ansible skeleton playbooks
├── job-templates/       # 2 AAP Job/Workflow templates
├── acm-policies/        # 3 ACM Policy/PlacementRule/AnsibleJob templates
└── manifests/
    ├── migration-policies/  # 5 MigrationPolicy + HCO examples
    └── rbac/                # ACM-AAP integration RBAC template
```

**Total:** 15 new skeleton files with placeholders for customization.

**Rationale:** Community of Practice deliverable needs concrete code examples, not just architecture. Skeletons provide starting points without assuming specific environment configs.

### CNV Operator Update Guides

Added dedicated documentation for OpenShift Virtualization (CNV) operator updates, separate from cluster upgrade orchestration.

**Key insight:** CNV updates are OLM-driven (automatic) while cluster upgrades require orchestration. The guides focus on configuration and monitoring rather than execution automation.

**Created:**
```
cnv-update/
├── README.md                    # Decision tree and overview
├── prerequisites.md             # Version compatibility, channel selection
├── hyperconverged-config.md     # workloadUpdateStrategy configuration
├── monitoring-upgrade.md        # Monitor progress and detect issues
├── control-plane-only.md        # OpenShift 4.16+ special handling
└── artifacts/
    ├── hyperconverged/          # 3 CR profiles (baseline, aggressive, conservative)
    └── verification/            # Shell script + Ansible playbook for monitoring
```

**Control Plane Only Updates:** OpenShift 4.16+ disables CNV workload updates during control-plane-only upgrades. Must re-enable and verify all workloads updated before proceeding.

**Integration with existing docs:** CNV update guides link to migration policy ADR and timeout calculation docs. Main README updated to include CNV update section.

---

## Graduation — 2026-07-17

**Status:** Graduated to `docs/guides/openshift-virtualization-upgrade/` and `artifacts/openshift/openshift-virtualization-upgrade/`

### Files Moved

**Documentation (→ `docs/guides/openshift-virtualization-upgrade/`):**
- `README.md` — Main entry point with decision tree
- `migration-timeout-calculation.md` — Timeout calibration reference
- `adr/0001-vm-policy-thresholds.md` — VM classification thresholds
- `execution-guides/` — 9 files (ansible-core, aap, aap-acm)
- `cnv-update/` — 5 docs + artifacts subdirectory

**Artifacts (→ `artifacts/openshift/openshift-virtualization-upgrade/`):**
- `playbooks/` — 4 Ansible skeleton playbooks
- `job-templates/` — 2 AAP templates
- `acm-policies/` — 3 ACM/AnsibleJob templates
- `manifests/migration-policies/` — 5 MigrationPolicy CRs
- `manifests/rbac/` — 1 RBAC template

**Total:** 26 files graduated

### Post-Graduation Updates

- Updated `docs/guides/README.md` with new index entry
- Removed session-specific document references from main README
- Updated relative paths in CNV artifacts
- Updated artifact paths to `artifacts/openshift/openshift-virtualization-upgrade/`

### Remaining in WIP

- `discussion.md` — Session log (this file)


