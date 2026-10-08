# Failure Pattern Reference — OpenShift Bare-Metal Installs

Read this file when `aap-failures.py` or `aap-scan-stdout.py` surfaces an error
you don't immediately recognise. The patterns below are all confirmed from real
production jobs on this codebase.

## Fast failures (< 5 min)

| Symptom | Root cause | Resolution |
|---|---|---|
| `invalid CIDR address: x.x.x/yy` | Missing last octet in `openshift_machine_network_subnet` | Fix cluster.yaml; `validate_variables` role verifies this format |
| `fatal` on `validate_variables` | Missing or invalid input var | Check required vars definition in role defaults / inventory documentation |
| `fatal` on `check_for_existing_install` | Prior install detected on nodes | Set `allow_destruction: <cluster-name>` in extra_vars |
| pip proxy failure at play setup | `proxy.example.com` connection reset | Infrastructure — retry; check proxy availability |
| `fatal` on `bastion_https_server` | Port 6183 already in use, nginx conflict, or SELinux | Check if a prior Phase 3 nginx is still running on bastion |

## Boot / iDRAC failures (5–25 min)

| Symptom | Root cause | Resolution |
|---|---|---|
| `fatal` on ISO mount / Redfish task | iDRAC unreachable or wrong `bare_metal_idrac_ip` | Verify iDRAC IPs in cluster.yaml; check network path to iDRAC VLAN |
| `BootProgress` stuck / no OSRunning | Node not POST-completing or ISO not booting | Check iDRAC console; verify ISO was mounted; check HDD boot override |
| `SSH readiness` exhausted | coreos-installer stalled or RHCOS didn't come up after install | Check eject timing; HDD/continuous boot override; boot loop |
| `unreachable` on all hosts | SSH credential issue, bastion unreachable, or wrong FQDN | Verify bastion FQDN and SSH key credential in AAP |
| `Monitor power state.*RETRYING` | Old code path (pre-BootProgress) running | Check `scm_revision` — likely running older unpatched revision |

## Install-time failures (25–90 min)

| Symptom | Root cause | Resolution |
|---|---|---|
| `NTO.*Progressing=True, Degraded=False` for 40–60 min | NTO profiles not applying | Ensure latest playbook profile fixes are present — check scm_revision |
| `rc=5` on openshift-install | Installer's own 1-hour timeout | Not necessarily fatal — check if API VIP is responding (aap-scan-stdout: look for API probe after rc=5) |
| `rc=7` on openshift-install | Ansible ASYNC_FAILED (Ansible killed the async task) | Phase 4 NTO post-failure recovery should trigger; check stdout for recovery attempt |
| Bootstrap complete but operators not converging | DNS, network path, or MTU issue post-bootstrap | Check network validation documentation and load balancer (F5/haproxy) config |

## Severe timeouts (> 90 min)

| Symptom | Root cause | Resolution |
|---|---|---|
| No `bootstrap-complete` signal | Nodes booted but can't reach bootstrap service | Check MTU, F5 routing, `use_haproxy_lb` matches VIP config |
| `Ironic` or BMH poller stuck | Metal³ provisioning stalled | Check Phase 4 BMH poller; verify iDRAC is responding post-boot |

## Infrastructure failures (< 1 min)

| Symptom | Root cause | Resolution |
|---|---|---|
| SCM sync failure | Git credential issue or unreachable GitHub | Check AAP project sync; verify PAT hasn't expired |
| Execution environment pull failure | Registry unreachable | Check `proxy.example.com` and registry credentials in AAP |
