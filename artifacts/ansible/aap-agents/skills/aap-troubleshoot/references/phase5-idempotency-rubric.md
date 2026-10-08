# phase_5 cancel/rerun idempotency rubric

Use this rubric whenever a `phase_5.yml` run is canceled, fails mid-flight, or a
user asks, "If I rerun, where will it pick up?"

## Inputs to inspect

1. AAP job metadata (`status`, `scm_revision`, elapsed, failed events).
2. Bastion phase 5 lock file: `~/.phase5_<cluster>.lock`.
3. Live cluster worker nodes from `oc get nodes -l node-role.kubernetes.io/worker`.
4. Requested inventory workers list for the run.

## Outcome classification

| Classification | Meaning | Typical trigger |
|---|---|---|
| `reprovision` | Rerun will execute node pre-provisioning again for some workers | Worker not in lock and not currently joined |
| `expand-only` | Rerun should skip provisioning and only continue expansion/waits | Worker already provisioned but not fully Ready |
| `no-op` | Rerun will intentionally do nothing | Workers already joined and lock/cluster both reflect completion |
| `unsafe-ambiguous` | State mismatch; rerun behavior not trustworthy without intervention | Lock says done, cluster state incomplete, or cluster query unavailable |

## Decision checks

1. **Can phase 5 query live cluster worker state?**
   - If no, classify as `unsafe-ambiguous` unless override variable explicitly permits continuation.
2. **Do lock entries align with live worker nodes?**
   - If lock contains workers not present/joined in cluster, classify as `unsafe-ambiguous`.
3. **Will `workers_to_provision` be empty?**
   - If yes and cluster still missing target workers, classify as `unsafe-ambiguous` (likely skip/no-op rerun).
4. **Are all target workers present and Ready?**
   - If yes, classify as `no-op`.

## Required output in troubleshooting response

Always include:

`cancel/rerun expectation: <classification> — <single sentence explanation>`

If `unsafe-ambiguous`, add:
- exact mismatch observed (lock vs cluster)
- safest remediation (e.g., correct lock entries, then rerun)
