# Cancel / rerun rubric

Use this whenever a job is canceled, fails mid-flight, or the user asks,
"If I rerun, where will it pick up?"

## Inputs to inspect

1. AAP job metadata (`status`, `scm_revision`, elapsed, failed events).
2. Any lock or checkpoint file the playbook documents for this workflow.
3. Live state of the target (what actually exists now).
4. The inventory or extra vars that say what this run intended to change.

## Outcome classification

| Classification | Meaning | Typical trigger |
|---|---|---|
| `reprovision` | Rerun will redo provisioning for some targets | Target not recorded as done and not currently in the desired state |
| `continue` | Rerun should skip completed work and continue the rest | Target already provisioned but not fully ready |
| `no-op` | Rerun will intentionally do nothing | Targets already match the desired state and the checkpoint agrees |
| `unsafe-ambiguous` | State mismatch; rerun behavior is not trustworthy without intervention | Checkpoint says done, live state does not, or live state cannot be queried |

## Decision checks

1. **Can the playbook query live target state?**
   - If no, classify as `unsafe-ambiguous` unless an override variable explicitly permits continuation.
2. **Do checkpoint entries align with live state?**
   - If the checkpoint lists targets that are not actually present or ready, classify as `unsafe-ambiguous`.
3. **Will the set of targets still to change be empty?**
   - If yes and the live target is still missing what the run asked for, classify as `unsafe-ambiguous` (the rerun would skip).
4. **Are all requested targets present and ready?**
   - If yes, classify as `no-op`.

## Required output in the troubleshooting response

Always include:

`cancel/rerun expectation: <classification> — <single sentence explanation>`

If `unsafe-ambiguous`, add:
- exact mismatch observed (checkpoint vs live state)
- safest remediation (correct the checkpoint, then rerun)
