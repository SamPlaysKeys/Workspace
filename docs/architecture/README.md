---
layout: page
title: Architecture & System Designs
category: Architecture
status: Active
---

This section compiles primary system architecture blueprints, integration designs, and infrastructure roadmaps.

## Architecture Blueprints

### Security & Integrations
- [Tailscale Integration for OpenBao](./openbao/tailscale-integration.md) - Security architecture for generating secure, on-demand Tailscale authorization keys for Dockerized containers using HashiCorp Vault / OpenBao.

### Deployment Orchestration
- [BigFix Discovery & Server Deployment](./openshift/bigfix-discovery-deployment.md) - Architectural design for rolling out BigFix server and discovery engines inside specialized namespaces.
- [BigFix Agent DaemonSet Deployment](./openshift/bigfix-agent-deployment.md) - DaemonSet design model for executing BigFix security agents on underlying OpenShift worker nodes.

### Consolidation Roadmaps
- [Infrastructure Consolidation Roadmap](./consolidation-roadmap.md) - Multi-phase migration plan for transitioning core homelab services and workloads from independent virtual machines to unified Kubernetes/OpenShift containers.
