---
type: Reference
---
# OpenShift Virtualization guides (documentation)

Prose guides for OpenShift Virtualization workflows used in this workspace.

## Guides in this directory

| Document | Topic |
|----------|--------|
| [Configuring Portworx for OpenShift Virtualization](portworx-setup.md) | Deploy Portworx Operator and configure a basic StorageCluster |
| [Portworx PVC Testing Walkthrough (RWX)](portworx-pvc-test.md) | Validate Portworx RWX volume sharing between pods |
| [PVC Testing Walkthrough — OpenShift Virtualization](pvc-test-guide.md) | Request and bind persistent storage using StorageClasses |
| [VM Affinity Balancing for Multi-Group Deployment](vm-affinity-balancing.md) | Configure node affinity and pod anti-affinity to distribute GRP01 and GRP02 VMs across 4 nodes |
| [VM Migration & Upgrade Strategy](openshift-virtualization-upgrade/README.md) | Minimize downtime during cluster upgrades with live migration tuning, VM classification, and orchestration workflows (Ansible, AAP, ACM) |

For other troubleshooting topics or general OpenShift administration, see **`docs/troubleshooting/`** and **`docs/guides/openshift/`**.
