# ==============================================================
# OpenBao Tailscale Auth Key — Dockerfile Template
# ==============================================================
# Add this to your application's Dockerfile so it fetches a
# Tailscale auth key from OpenBao at container startup.
#
# Runtime env vars required:
#   vault_address  - OpenBao server URL (e.g. http://openbao:8200)
#   bao_token      - OpenBao authentication token
#   service_name   - Name for this service (tracked in key description)
# ==============================================================

# Install dependencies (Alpine, adjust for your base image)
RUN apk add --no-cache curl jq tailscale

# Entrypoint: fetch auth key from OpenBao, authenticate with
# Tailscale, then run the application.
RUN cat > /entrypoint-tailscale.sh <<'SCRIPT'
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

tailscaled --tun=userspace-networking &
sleep 2
tailscale up --auth-key="$auth_key"

echo "Tailscale authenticated. Starting application..."
exec "$@"
SCRIPT
RUN chmod +x /entrypoint-tailscale.sh

ENTRYPOINT ["/entrypoint-tailscale.sh"]
# Keep your existing CMD — the entrypoint will exec into it
