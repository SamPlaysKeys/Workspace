---
type: Reference
---

# Reference Document: Ansible Automation Platform (AAP) Path

This document provides Job Template schemas, REST API references, Workflow constructs, and links to external documentation for the AAP execution path.

---

## AAP Controller API Reference

### Authentication

AAP API supports token-based authentication. Tokens can be user tokens or OAuth2 application tokens.

**Create User Token:**
```bash
# Via UI: Users > <user> > Tokens > Add
# Via API:
curl -X POST https://<aap-controller>/api/v2/tokens/ \
  -u <username>:<password> \
  -H "Content-Type: application/json"
```

**Use Token:**
```bash
curl https://<aap-controller>/api/v2/ \
  -H "Authorization: Bearer <token>"
```

**API Documentation:** https://docs.ansible.com/automation-controller/latest/html/controllerapi/

---

## Core AAP Resources

### Project

Defines the source of Ansible content (Git repository).

**API Endpoint:** `/api/v2/projects/`

**Create Project:**
```json
{
  "name": "openshift-virt-playbooks",
  "description": "OpenShift Virtualization migration playbooks",
  "organization": 1,
  "scm_type": "git",
  "scm_url": "https://github.com/org/openshift-virt-automation.git",
  "scm_branch": "main",
  "credential": 5,
  "execution_environment": 3
}
```

**Key Fields:**
| Field | Type | Description |
|-------|------|-------------|
| `scm_type` | string | Source control type (`git`, `svn`, `archive`) |
| `scm_url` | string | Repository URL |
| `scm_branch` | string | Branch name |
| `credential` | integer | Git credential ID |
| `execution_environment` | integer | EE ID to use |

**Sync Project:**
```bash
curl -X POST https://<aap>/api/v2/projects/<id>/update/ \
  -H "Authorization: Bearer <token>"
```

---

### Credential

Stores authentication secrets for target systems.

**API Endpoint:** `/api/v2/credentials/`

**OpenShift/Kubernetes Credential:**
```json
{
  "name": "openshift-prod-credential",
  "description": "OpenShift production cluster",
  "organization": 1,
  "credential_type": 17,
  "inputs": {
    "host": "https://api.cluster.example.com:6443",
    "bearer_token": "<service-account-token>",
    "verify_ssl": true
  }
}
```

**Credential Types:**
| ID | Type | Use Case |
|----|------|----------|
| 1 | Machine | SSH credentials |
| 17 | Kubernetes / OpenShift | API token auth |
| 19 | Vault | HashiCorp Vault |
| 20 | Git | Git SCM auth |

---

### Inventory

Defines target hosts and groups.

**API Endpoint:** `/api/v2/inventories/`

**Create Inventory:**
```json
{
  "name": "openshift-clusters",
  "description": "OpenShift cluster inventory",
  "organization": 1
}
```

**Add Host:**
```json
{
  "name": "cluster-prod",
  "inventory": 1,
  "variables": {
    "ansible_connection": "local",
    "openshift_api_url": "https://api.cluster.example.com:6443",
    "openshift_namespace": "openshift-cnv"
  }
}
```

---

### Job Template

Defines how to run a single playbook.

**API Endpoint:** `/api/v2/job_templates/`

**Create Job Template:**
```json
{
  "name": "vm-live-migrate",
  "description": "Live migrate VMs off target node",
  "job_type": "run",
  "inventory": 1,
  "project": 2,
  "playbook": "playbooks/vm_live_migrate.yml",
  "credential": 3,
  "execution_environment": 3,
  "extra_vars": "{}",
  "ask_variables_on_launch": true,
  "survey_enabled": true,
  "timeout": 3600
}
```

**Key Fields:**
| Field | Type | Description |
|-------|------|-------------|
| `job_type` | string | `run` or `check` |
| `playbook` | string | Path to playbook in project |
| `ask_variables_on_launch` | boolean | Allow extra_vars on launch |
| `survey_enabled` | boolean | Enable survey (form) inputs |
| `timeout` | integer | Job timeout in seconds |

**Survey Spec:**
```json
{
  "name": "",
  "description": "",
  "spec": [
    {
      "type": "text",
      "question_name": "Target Node",
      "question_description": "Node to evacuate and upgrade",
      "variable": "target_node",
      "required": true
    },
    {
      "type": "text",
      "question_name": "Namespace",
      "question_description": "Namespace to scan for VMs",
      "variable": "namespace",
      "required": false,
      "default": "openshift-cnv"
    }
  ]
}
```

---

### Workflow Job Template

Defines a DAG of Job Templates.

**API Endpoint:** `/api/v2/workflow_job_templates/`

**Create Workflow:**
```json
{
  "name": "vm-migration-upgrade-workflow",
  "description": "Full upgrade workflow for OpenShift Virtualization nodes",
  "organization": 1,
  "extra_vars": "{}",
  "ask_variables_on_launch": true,
  "survey_enabled": true
}
```

**Workflow Nodes:**
Workflows are constructed via the `/api/v2/workflow_job_template_nodes/` endpoint. Each node references a Job Template and defines success/failure edges.

**Create Workflow Node:**
```json
{
  "workflow_job_template": 1,
  "unified_job_template": 5,
  "success_nodes": [2, 3],
  "failure_nodes": [4]
}
```

---

### Launch Workflow

**API Endpoint:** `/api/v2/workflow_job_templates/<id>/launch/`

**Launch Request:**
```bash
curl -X POST https://<aap>/api/v2/workflow_job_templates/1/launch/ \
  -H "Authorization: Bearer <token>" \
  -H "Content-Type: application/json" \
  -d '{
    "extra_vars": {
      "target_node": "worker-01",
      "maintenance_window_id": "CHG-12345"
    }
  }'
```

**Response:**
```json
{
  "id": 42,
  "name": "vm-migration-upgrade-workflow",
  "status": "pending",
  "url": "/api/v2/workflow_jobs/42/"
}
```

---

### Check Workflow Status

**API Endpoint:** `/api/v2/workflow_jobs/<id>/`

```bash
curl https://<aap>/api/v2/workflow_jobs/42/ \
  -H "Authorization: Bearer <token>"
```

**Response Fields:**
| Field | Description |
|-------|-------------|
| `status` | `pending`, `running`, `successful`, `failed`, `canceled` |
| `started` | ISO 8601 timestamp |
| `finished` | ISO 8601 timestamp (null if running) |
| `elapsed` | Seconds elapsed |

---

## Webhook Configuration

### GitHub Webhook

**URL Format:** `https://<aap>/api/v2/workflow_job_templates/<id>/github/`

**Configure in GitHub:**
1. Go to Repository > Settings > Webhooks.
2. Add webhook URL.
3. Set Content-Type to `application/json`.
4. Select events: `push` or `pull_request`.

**Payload Processing:**
AAP automatically extracts branch, commit SHA, and repository info. Custom logic can be added in playbook.

---

### GitLab Webhook

**URL Format:** `https://<aap>/api/v2/workflow_job_templates/<id>/gitlab/`

---

### Generic Webhook

**URL Format:** `https://<aap>/api/v2/workflow_job_templates/<id>/generic/`

**Custom Payload:**
```json
{
  "extra_vars": {
    "target_node": "worker-01"
  }
}
```

---

## Execution Environment Reference

### ansible-builder Definition

**File:** `execution-environment.yml`

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
      - name: community.general
        version: ">=8.0.0"
  python:
    - openshift
    - kubernetes>=26.0.0
    - pyyaml
  system:
    - git
additional_build_steps:
  append_base:
    - RUN alternatives --set python /usr/bin/python3
```

**Build Command:**
```bash
ansible-builder build -t quay.io/org/openshift-virt-ee:latest -v 3
```

**Push to Registry:**
```bash
podman push quay.io/org/openshift-virt-ee:latest
```

---

## Sample Playbook: vm_live_migrate.yml

```yaml
---
- name: Live migrate VMs from target node
  hosts: localhost
  gather_facts: false
  vars:
    target_node: ""
    namespace: "openshift-cnv"
    migration_timeout: 1800

  tasks:
    - name: Get VMIs on target node
      kubernetes.core.k8s_info:
        api_version: kubevirt.io/v1
        kind: VirtualMachineInstance
        namespace: "{{ namespace }}"
        label_selectors:
          - "kubevirt.io/nodeName={{ target_node }}"
      register: vmi_list

    - name: Create migration CR for each VMI
      kubernetes.core.k8s:
        state: present
        definition:
          apiVersion: kubevirt.io/v1
          kind: VirtualMachineInstanceMigration
          metadata:
            name: "migrate-{{ item.metadata.name }}"
            namespace: "{{ namespace }}"
          spec:
            vmiName: "{{ item.metadata.name }}"
      loop: "{{ vmi_list.resources }}"
      register: migration_cr

    - name: Wait for migrations to complete
      kubernetes.core.k8s_info:
        api_version: kubevirt.io/v1
        kind: VirtualMachineInstanceMigration
        name: "migrate-{{ item.metadata.name }}"
        namespace: "{{ namespace }}"
      loop: "{{ vmi_list.resources }}"
      register: migration_status
      until: migration_status.resources[0].status.phase in ["Succeeded", "Failed"]
      retries: "{{ migration_timeout // 30 }}"
      delay: 30

    - name: Check for failed migrations
      fail:
        msg: "Migration {{ item.metadata.name }} failed"
      when: migration_status.results[0].resources[0].status.phase == "Failed"
      loop: "{{ vmi_list.resources }}"
```

---

## External Documentation Links

### AAP Controller

| Topic | Link |
|-------|------|
| AAP Controller Documentation | https://docs.ansible.com/automation-controller/latest/html/controlleradmin/ |
| API Reference | https://docs.ansible.com/automation-controller/latest/html/controllerapi/ |
| Workflow Visualizer | https://docs.ansible.com/automation-controller/latest/html/controlleruser/workflows.html |
| Execution Environments | https://docs.ansible.com/automation-controller/latest/html/controlleradmin/execution_environments.html |
| Credentials Management | https://docs.ansible.com/automation-controller/latest/html/controlleruser/credentials.html |
| Webhooks | https://docs.ansible.com/automation-controller/latest/html/controlleruser/webhooks.html |

### ansible-builder

| Topic | Link |
|-------|------|
| ansible-builder Documentation | https://ansible-builder.readthedocs.io/ |
| Creating Execution Environments | https://ansible-builder.readthedocs.io/en/stable/definition/ |

### OpenShift Virtualization

| Topic | Link |
|-------|------|
| Live Migration | https://docs.openshift.com/container-platform/latest/virt/virtual_machines/virt-node-maintenance-virt.html |
| Migration Policies | https://docs.openshift.com/container-platform/latest/virt/virtual_machines/virt-node-maintenance-virt.html#virt-configuring-live-migration-policies_virt-node-maintenance-virt |

### Ansible Collections

| Topic | Link |
|-------|------|
| kubernetes.core | https://docs.ansible.com/ansible/latest/collections/kubernetes/core/ |
| kubernetes.core.k8s | https://docs.ansible.com/ansible/latest/collections/kubernetes/core/k8s_module.html |
| kubernetes.core.k8s_info | https://docs.ansible.com/ansible/latest/collections/kubernetes/core/k8s_info_module.html |
| kubernetes.core.k8s_drain | https://docs.ansible.com/ansible/latest/collections/kubernetes/core/k8s_drain_module.html |

---

## Common API Operations

### List All Job Templates
```bash
curl https://<aap>/api/v2/job_templates/ \
  -H "Authorization: Bearer <token>" | jq '.results[].name'
```

### Get Job Log
```bash
curl https://<aap>/api/v2/jobs/<id>/stdout/ \
  -H "Authorization: Bearer <token>"
```

### Cancel Running Job
```bash
curl -X POST https://<aap>/api/v2/jobs/<id>/cancel/ \
  -H "Authorization: Bearer <token>"
```

### Relaunch Failed Job
```bash
curl -X POST https://<aap>/api/v2/jobs/<id>/relaunch/ \
  -H "Authorization: Bearer <token>"
```

### Get Workflow Job Nodes
```bash
curl https://<aap>/api/v2/workflow_jobs/<id>/workflow_nodes/ \
  -H "Authorization: Bearer <token>" | jq '.results[] | {name: .unified_job_template_name, status: .job.status}'
```
