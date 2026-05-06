# Komodo Artifact Templates — Discussion

## 2026-05-05

**Initial questions:**
- What are the most common Komodo use cases to template? CI/CD pipelines, scheduled jobs, and service definitions are likely candidates.
- Should we include annotated examples for validation (e.g., JSON schema checks) and secrets handling (e.g., SOPS)?
- How to structure templates for composability? (e.g., modular includes)

**Decision needed:**
- Whether to include a `komodo.yml` (global config) alongside service-specific TOMLs. This would help users see the full picture.

---

Draft outline for `artifacts/komodo/`: `
komodo/
├── cicd/
│   ├── basic-pipeline.toml
│   └── scheduled-job.toml
├── services/
│   ├── static-site.toml
│   └── rest-api.toml
├── komodo.yml      # Optional: global configuration
└── README.md
`