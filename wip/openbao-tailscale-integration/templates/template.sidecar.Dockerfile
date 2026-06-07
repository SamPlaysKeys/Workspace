# ==============================================================
# OpenBao Tailscale Sidecar — Dockerfile
# ==============================================================
# For the sidecar pattern: a standalone container that fetches
# a Tailscale auth key from OpenBao at startup, authenticates,
# and keeps tailscaled running. App containers attach to this
# sidecar's network to access the tailnet.
#
# Runtime env vars required:
#   vault_address  - OpenBao server URL (e.g. http://openbao:8200)
#   bao_token      - OpenBao authentication token
#   service_name   - Name for this service (tracked in key description)
# ==============================================================

FROM alpine:3.18

RUN apk add --no-cache curl jq tailscale

COPY <<'SCRIPT' /entrypoint.sh
#!/bin/sh
set -e

echo "Requesting Tailscale auth key from OpenBao..."
auth_key=$(curl -s --header "X-Vault-Token: ${bao_token}" \
  "${vault_address}/v1/docker/tailscale/auth-token/${service_name}" \
  | jq -r '.data.auth_token')

if [ -z "$auth_key" ] || [ "$auth_key" = "null" ]; then
  echo "FATAL: Failed to retrieve Tailscale auth key"
  exit 1
fi

echo "Starting tailscaled..."
tailscaled --tun=userspace-networking &
sleep 2

echo "Authenticating with Tailscale..."
tailscale up --auth-key="$auth_key"
echo "Sidecar connected to tailnet."

# Keep container alive so the network namespace persists
wait
SCRIPT

RUN chmod +x /entrypoint.sh

ENTRYPOINT ["/entrypoint.sh"]
