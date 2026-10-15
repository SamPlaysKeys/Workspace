# Failure Pattern Reference

Read this file when `aap-failures.py` or `aap-scan-stdout.py` surfaces an error
you don't immediately recognise. The rows are categories. Substitute the task
names, variable names, and ports from the playbook that actually ran.

## Fast failures (< 5 min)

| Symptom | Root cause | Resolution |
|---|---|---|
| Invalid address or CIDR in the error | Malformed network input | Fix the variable named in the error; confirm the playbook's validation task |
| `fatal` on a validate / preflight task | Missing or invalid input var | Check required vars in role defaults or inventory |
| `fatal` on an existing-state check | Prior run left state the play refuses to overwrite | Set the playbook's explicit override variable, if it has one |
| Dependency or proxy failure at play setup | Proxy or mirror unreachable | Infrastructure — retry; check proxy availability |
| `fatal` on a helper service (port in use) | A previous run left a process bound to the playbook's listen port | Check the helper host for a leftover process |

## Boot / BMC failures (5–25 min)

| Symptom | Root cause | Resolution |
|---|---|---|
| `fatal` on virtual media or Redfish task | BMC unreachable or wrong BMC address | Verify BMC addresses; check the network path |
| Boot progress stuck | Node not completing POST, or install media not booting | Check the BMC console and boot override |
| SSH readiness exhausted | Installer stalled after boot | Check media eject timing and boot device order |
| `unreachable` on all hosts | SSH credential, jump host, or wrong name | Verify the jump host and the SSH credential in AAP |
| Power-state polling still retrying | Older revision still using a polling loop | Check `scm_revision` |

## Install-time failures (25–90 min)

| Symptom | Root cause | Resolution |
|---|---|---|
| A condition stuck progressing for a long time | A configuration or operator is not applying | Confirm the revision that contains the fix actually ran |
| Installer return code for its own timeout | The installer gave up on its timer | Not necessarily fatal — check whether the service is up after that code |
| Ansible `ASYNC_FAILED` | Ansible killed the async task | Look in stdout for a recovery task |
| Early install succeeded, later components not converging | DNS, path, MTU, or load balancer | Check network validation and VIP / load-balancer config |

## Severe timeouts (> 90 min)

| Symptom | Root cause | Resolution |
|---|---|---|
| No ready / bootstrap-complete signal | Nodes came up but cannot reach the service they need | Check MTU and whether load-balancer config matches the VIP |
| Provisioning poller stuck | Hardware provisioning stalled | Check the poller task and whether the BMC still answers |

## Infrastructure failures (< 1 min)

| Symptom | Root cause | Resolution |
|---|---|---|
| SCM sync failure | Git credential or unreachable Git host | Check AAP project sync; verify the credential |
| Execution environment pull failure | Registry unreachable | Check proxy and registry credentials in AAP |
