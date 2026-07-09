# Context: DevOps-Learning-Plan

**Goal**: Build a reusable DevOps learning path usable for (a) upskilling existing staff and (b) onboarding new hires. Must be portable to Confluence, with clear slots for org-specific content.
**Current State**:
- Source material pulled from github.com/hhellbusch/my-ai-workspace (Red Hat / OpenShift-centric).
- Scratch reference copies saved to `scratch/` (00-index, 01-git path, 02-vmware-admins→k8s/ocp path).
- Outcome defined: upskill at work + onboarding. Pace/format: self-paced, Confluence-ready.
- Index + 10 module pages built in `curriculum/`.
- ALL 10 modules now open with a `## Checklist — work through in order` (`- [ ]` boxes) and enriched public training/cert recs (GitHub Foundations, RH124/RHCSA, CNCF KCNA/CKA/CKAD/CKS, DO180/280/316/480, EX280/EX480, Akuity/KodeKloud, kubevirt.io, Prometheus/Grafana). ORG-SPECIFIC slots preserved throughout.
**Source content inventory**:
- Git/GitHub/GitLab learning path (mental model → hands-on → internals; change-management reframe).
- VMware admin → K8s/OCP/OCP Virt path (Phases 0–5 + ZTP specialist + certs + disconnected appendix).
**Decided structure**:
- Deliverable = **Index page + one page per module** (Confluence-ready).
- Curriculum follows a **DevOps spine** with named modules (see below).
- Each module = Audience, Outcomes, Topics, Resources, Scenario-based Verification, + `<!-- ORG-SPECIFIC -->` slots.
- Lean on **public training/certifications**: Module 1 includes Microsoft "GitHub Fundamentals" learn path + **GitHub Foundations** certification.
**Open Questions**:
- Where do org specifics (internal clusters, tooling, runbooks) slot in? → resolved via ORG-SPECIFIC slots per module.
**Key Constraints**:
- Must remain portable to Confluence (plain markdown, minimal external tooling).
- Org-specifics are added later, not invented now.
- Source is Red Hat/OCP-centric; keep that depth but frame as internal path.
