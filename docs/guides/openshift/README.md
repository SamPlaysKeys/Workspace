---
type: Reference
---
# OpenShift guides (documentation)

Prose guides for OpenShift Container Platform workflows used in this workspace. Executable templates and Ansible patterns live under **`artifacts/openshift/`** instead of here.

## Guides in this directory

| Document | Topic |
|----------|--------|
| [Checking OpenShift Certificates](checking-certificates.md) | Verify expiration dates of API, Ingress, and other internal certificates |
| [Using the integrated OpenShift container image registry](using-integrated-openshift-registry.md) | Push/pull, in-cluster image references |
| [NVIDIA vGPU Manager: integrated registry and ClusterPolicy](nvidia-vgpu-manager-internal-registry-and-clusterpolicy.md) | Build vGPU Manager image, integrated registry, GPU Operator `ClusterPolicy` |
| [BigFix Host Agent Client Deployment via GitOps](bigfix-agent-deployment.md) | Deploy host-level auditing, patching, and discovery DaemonSet onto RHCOS / RHEL nodes |
| [BigFix Inventory (BFI) Discovery & NFD Deployment via GitOps](bigfix-discovery-deployment.md) | Deploy Node Feature Discovery (NFD) operator/instance and API token/RBAC mappings for sub-capacity licensing calculations |

## Related artifacts

- **[Ansible OpenShift readiness / validation suite](../../../artifacts/openshift/readiness-validation-ansible/README.md)** — Multi-play parent playbook, `readiness_*` role conventions, PASS/WARN/FAIL reporting, examples (`check_node_readiness`, `check_storage_health`), and [`rules.md`](../../../artifacts/openshift/readiness-validation-ansible/rules.md) for agent-friendly constraints.

For other troubleshooting topics (including container image tagging), see **`docs/troubleshooting/`**.
