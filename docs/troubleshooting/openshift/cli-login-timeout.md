# oc login Timeout (Web Console Works)

## Symptoms

- `oc login` times out or fails to connect
- `oc login --web` and `oc login --token` also fail
- Web console login works fine
- `curl -k https://<api-server>:6443` times out

## Cause

Proxy or firewall blocking CLI access to the OpenShift API server. Browser routes through a proxy with access; terminal does not.

## Fix

**Option 1: Use a jump server**

SSH to a jump server with direct cluster network access, then:

```bash
oc login --web
```

**Option 2: Configure proxy in terminal**

```bash
export HTTPS_PROXY=http://your-proxy:port
export NO_PROXY=.cluster.local,localhost
oc login <api-server>
```

**Option 3: SSH tunnel**

```bash
ssh -L 6443:<api-server>:6443 user@jump-server
# Then in another terminal:
oc login https://localhost:6443
```

## Verification

```bash
oc whoami
oc get projects
```
