# Context: OCP/Operator Versioning Strategy

**Goal**: Develop a new versioning strategy for Helm component patterns to manage operator dependencies across multiple OpenShift Container Platform (OCP) versions.
**Current State**: Current versioning is operator-based (sets of upgraded operators tied to specific tags).
**Open Questions**: How to decouple operator versions from OCP versions while maintaining compatibility mapping.
**Key Constraints**: Must support multiple OCP versions simultaneously. Must integrate with existing operator-based versioning/tagging.

