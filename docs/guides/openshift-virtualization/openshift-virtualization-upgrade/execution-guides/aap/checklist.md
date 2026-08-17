---
type: Guide
---

# Execution Checklist: Ansible Automation Platform (AAP) Path

This checklist provides a step-by-step guide for configuring AAP to orchestrate OpenShift Virtualization VM migrations and node upgrades. Complete each phase in order.

---

## Phase 0: Pre-Execution Setup (AAP Configuration)

### 0.1 Execution Environment

| Step | Action | Verification |
|------|--------|--------------|
| 0.1.1 | Create `execution-environment.yml` definition file | File created |
| 0.1.2 | Include `kubernetes.core` and `openshift` Python libraries in EE | Dependencies listed |
| 0.1.3 | Build EE using `ansible-builder` | Image built |
| 0.1.4 | Push EE to container registry (e.g., Quay, OpenShift registry) | Image pushed |
| 0.1.5 | Create EE in AAP Controller (Organizations > Execution Environments) | EE visible in AAP |

**Sample `execution-environment.yml`:**
```yaml
version: 3
images:
  base_image:
    name: registry.redhat.io/ansible-automation-platform-24/ee-minimal-rhel8:latest
dependencies:
  galaxy:
    collections:
      - name: kubernetes.core
        version: ">=2.4.0"
  python:
    - openshift
    - kubernetes
```

### 0.2 Git Project

| Step | Action | Verification |
|------|--------|--------------|
| 0.2.1 | Create Git repository for playbooks | Repo created |
| 0.2.2 | Structure: `playbooks/`, `roles/`, `inventory/` | Structure defined |
| 0.2.3 | Create AAP Project (Resources > Projects) | Project created |
| 0.2.4 | Configure Git SCM URL and branch | URL and branch set |
| 0.2.5 | Configure credential for Git (SSH key or PAT) | Credential attached |
| 0.2.6 | Sync Project | Project synced successfully |

### 0.3 OpenShift Credentials

| Step | Action | Verification |
|------|--------|--------------|
| 0.3.1 | Create ServiceAccount in OpenShift: `oc create sa aap-automation -n openshift-automation` | SA created |
| 0.3.2 | Grant cluster-admin: `oc adm policy add-cluster-role-to-user cluster-admin -z aap-automation -n openshift-automation` | Role bound |
| 0.3.3 | Extract token: `oc create token aap-automation -n openshift-automation --duration=87600h` | Token captured |
| 0.3.4 | Create Credential in AAP (Resources > Credentials) with type `OpenShift` or `Kubernetes` | Credential created |
| 0.3.5 | Enter OpenShift API URL and token | API URL and token set |
| 0.3.6 | Test credential by running a simple job | Connection successful |

### 0.4 Inventory

| Step | Action | Verification |
|------|--------|--------------|
| 0.4.1 | Create Inventory in AAP (Resources > Inventories) | Inventory created |
| 0.4.2 | Add host entry for OpenShift API endpoint | Host added |
| 0.4.3 | Set host variables: `ansible_connection: local`, `openshift_api_url`, `openshift_namespace` | Variables set |
| 0.4.4 | Alternatively, configure dynamic inventory (AAP or external) | Inventory populated |

---

## Phase 1: Job Template Creation

### 1.1 Pre-Migration Assessment Template

| Step | Action | Verification |
|------|--------|--------------|
| 1.1.1 | Create Job Template: `vm-pre-migration-assessment` | Template created |
| 1.1.2 | Select Project, Inventory, Credential, Execution Environment | All selected |
| 1.1.3 | Set playbook path: `playbooks/vm_pre_migration_assessment.yml` | Path set |
| 1.1.4 | Add Survey for inputs: `target_node`, `namespace` | Survey created |
| 1.1.5 | Enable "Prompt on Launch" for `extra_vars` if needed | Prompt enabled |

**Survey Inputs:**
| Variable | Type | Required | Description |
|----------|------|----------|-------------|
| `target_node` | Text | Yes | Node to upgrade |
| `namespace` | Text | No | Namespace filter (default: all) |

### 1.2 Apply Migration Policy Template

| Step | Action | Verification |
|------|--------|--------------|
| 1.2.1 | Create Job Template: `apply-migration-policy` | Template created |
| 1.2.2 | Set playbook: `playbooks/apply_migration_policy.yml` | Path set |
| 1.2.3 | Add Survey: `vm_class`, `policy_name` | Survey created |

### 1.3 Live Migrate VMs Template

| Step | Action | Verification |
|------|--------|--------------|
| 1.3.1 | Create Job Template: `vm-live-migrate` | Template created |
| 1.3.2 | Set playbook: `playbooks/vm_live_migrate.yml` | Path set |
| 1.3.3 | Add Survey: `target_node`, `vm_list` (optional) | Survey created |
| 1.3.4 | Set job timeout (e.g., 3600 seconds for long migrations) | Timeout set |

### 1.4 Drain Node Template

| Step | Action | Verification |
|------|--------|--------------|
| 1.4.1 | Create Job Template: `node-drain` | Template created |
| 1.4.2 | Set playbook: `playbooks/node_drain.yml` | Path set |
| 1.4.3 | Add Survey: `target_node`, `drain_timeout` | Survey created |

### 1.5 Upgrade Node Template

| Step | Action | Verification |
|------|--------|--------------|
| 1.5.1 | Create Job Template: `node-upgrade` | Template created |
| 1.5.2 | Set playbook: `playbooks/node_upgrade.yml` | Path set |
| 1.5.3 | Add Survey: `target_node`, `upgrade_version` (optional) | Survey created |

### 1.6 Validate Node Template

| Step | Action | Verification |
|------|--------|--------------|
| 1.6.1 | Create Job Template: `node-validate` | Template created |
| 1.6.2 | Set playbook: `playbooks/node_validate.yml` | Path set |
| 1.6.3 | Add Survey: `target_node` | Survey created |

### 1.7 Rollback Template

| Step | Action | Verification |
|------|--------|--------------|
| 1.7.1 | Create Job Template: `node-rollback` | Template created |
| 1.7.2 | Set playbook: `playbooks/node_rollback.yml` | Path set |
| 1.7.3 | Add Survey: `target_node`, `rollback_action` (uncordon, cancel_migration) | Survey created |

---

## Phase 2: Workflow Job Template Creation

| Step | Action | Verification |
|------|--------|--------------|
| 2.1 | Create Workflow Job Template: `vm-migration-upgrade-workflow` | Workflow created |
| 2.2 | Open Workflow Visualizer | Visualizer open |
| 2.3 | Add START node | Node added |
| 2.4 | Add `vm-pre-migration-assessment` Job Template, connect from START | Node connected |
| 2.5 | Add `apply-migration-policy` Job Template, connect on SUCCESS | Node connected |
| 2.6 | Add `vm-live-migrate` Job Template, connect on SUCCESS | Node connected |
| 2.7 | Add `node-drain` Job Template, connect on SUCCESS | Node connected |
| 2.8 | Add `node-upgrade` Job Template, connect on SUCCESS | Node connected |
| 2.9 | Add `node-validate` Job Template, connect on SUCCESS | Node connected |
| 2.10 | Add END node, connect from `node-validate` on SUCCESS | Workflow complete |
| 2.11 | Add `node-rollback` Job Template, connect from any node on FAILURE | Rollback path defined |
| 2.12 | Add `send-alert` Job Template (optional), connect on FAILURE | Alerting configured |
| 2.13 | Configure Survey on Workflow to collect `target_node` | Survey created |
| 2.14 | Save Workflow | Workflow saved |

**Workflow Survey:**
| Variable | Type | Required | Description |
|----------|------|----------|-------------|
| `target_node` | Text | Yes | Node to upgrade |
| `maintenance_window_id` | Text | No | ITSM change ticket ID |

---

## Phase 3: GitOps Trigger Configuration

### Option A: Webhook Trigger

| Step | Action | Verification |
|------|--------|--------------|
| 3.1.1 | In Workflow Job Template, enable Webhook | Webhook enabled |
| 3.1.2 | Copy Webhook URL | URL copied |
| 3.1.3 | Add Webhook Key (optional, for authentication) | Key set |
| 3.1.4 | Configure Git platform webhook (GitHub/GitLab) to send POST to URL | Webhook configured |
| 3.1.5 | Test webhook: push to Git branch | Workflow launched |

**Webhook URL Format:**
```
https://<aap-controller>/api/v2/workflow_job_templates/<id>/github/
```

### Option B: REST API Trigger

| Step | Action | Verification |
|------|--------|--------------|
| 3.2.1 | Create AAP User token (Users > <user> > Tokens) | Token created |
| 3.2.2 | Store token securely (vault, secrets manager) | Token stored |
| 3.2.3 | Construct API call payload | Payload ready |

**API Call Example:**
```bash
curl -X POST \
  https://<aap-controller>/api/v2/workflow_job_templates/<id>/launch/ \
  -H "Authorization: Bearer <token>" \
  -H "Content-Type: application/json" \
  -d '{
    "extra_vars": {
      "target_node": "worker-01",
      "maintenance_window_id": "CHG-12345"
    }
  }'
```

### Option C: Schedule Trigger

| Step | Action | Verification |
|------|--------|--------------|
| 3.3.1 | Create Schedule in Workflow Job Template (Schedules tab) | Schedule created |
| 3.3.2 | Set cron expression (e.g., `0 2 * * 0` for Sundays 2 AM) | Expression set |
| 3.3.3 | Set extra_vars for scheduled run | Variables set |
| 3.3.4 | Enable schedule | Schedule enabled |

---

## Phase 4: RBAC Configuration

| Step | Action | Verification |
|------|--------|--------------|
| 4.1 | Create Team: `openshift-ops` | Team created |
| 4.2 | Create Team: `openshift-admin` | Team created |
| 4.3 | Create Team: `platform-auditor` | Team created |
| 4.4 | Assign users to teams | Users assigned |
| 4.5 | Create Role: `Workflow Executor` (execute permission on workflow) | Role created |
| 4.6 | Assign `Workflow Executor` to `openshift-ops` team | Role assigned |
| 4.7 | Create Role: `Job Template Admin` (admin permission on templates) | Role created |
| 4.8 | Assign `Job Template Admin` to `openshift-admin` team | Role assigned |
| 4.9 | Create Role: `Auditor` (view permission on jobs, workflows) | Role created |
| 4.10 | Assign `Auditor` to `platform-auditor` team | Role assigned |

---

## Phase 5: Execute Upgrade Workflow

| Step | Action | Verification |
|------|--------|--------------|
| 5.1 | Navigate to Workflow Job Template: `vm-migration-upgrade-workflow` | Template visible |
| 5.2 | Click "Launch" | Launch dialog opens |
| 5.3 | Fill Survey inputs: `target_node`, `maintenance_window_id` | Inputs provided |
| 5.4 | Click "Launch" | Workflow starts |
| 5.5 | Monitor workflow in "Jobs" view | Job status visible |
| 5.6 | Verify each node completes successfully | All nodes green |
| 5.7 | Review job logs if any node fails | Logs reviewed |
| 5.8 | If rollback triggered, verify rollback completion | Rollback complete |
| 5.9 | Download job log for record keeping | Log archived |

---

## Phase 6: Post-Execution Review

| Step | Action | Verification |
|------|--------|--------------|
| 6.1 | Review audit log in AAP (Access > Audit) | Audit log reviewed |
| 6.2 | Verify workflow launched by correct user | User confirmed |
| 6.3 | Check job duration for optimization | Duration recorded |
| 6.4 | Export workflow run details for ITSM ticket | Export complete |
| 6.5 | Update runbook with any policy adjustments | Runbook updated |

---

## Troubleshooting Guide

| Issue | Detection | Resolution |
|-------|-----------|------------|
| Project sync fails | Project status "Failed" | Check Git URL, credential, network connectivity |
| Credential auth fails | Job fails with 401/403 | Rotate token, update credential in AAP |
| EE import error | Job fails on `import` | Rebuild EE with missing dependencies |
| Workflow stuck | Job status "Running" indefinitely | Check node capacity, cancel and retry |
| Webhook not triggering | No workflow launch on push | Verify webhook URL, key, and Git platform config |
| Survey not prompting | No input dialog on launch | Check Survey is enabled on template |

---

## Quick Reference: AAP CLI Commands

```bash
# Launch workflow via API
curl -X POST https://<aap>/api/v2/workflow_job_templates/<id>/launch/ \
  -H "Authorization: Bearer <token>" \
  -H "Content-Type: application/json" \
  -d '{"extra_vars": {"target_node": "worker-01"}}'

# Check workflow job status
curl https://<aap>/api/v2/workflow_jobs/<id>/ \
  -H "Authorization: Bearer <token>"

# Get job log
curl https://<aap>/api/v2/jobs/<id>/stdout/ \
  -H "Authorization: Bearer <token>"
```
