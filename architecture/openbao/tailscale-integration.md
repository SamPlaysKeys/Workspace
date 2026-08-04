---
type: Guide
status: Active
system: OpenBao
related_to:
- docs/homelab/network/tailscale.md
references:
- Tailscale Keys API Documentation: https://tailscale.com/kb/1101/api#keys
- OpenBao Plugin Development: https://openbao.org/docs/plugins/developing/
category: Architecture
layout: page
title: Tailscale Integration for OpenBao
---
{% raw %}


# Tailscale Integration for OpenBao

**Plugin repository:** <https://github.com/SamPlaysKeys/openbao-plugin-secrets-tailscale>

This guide details how to set up an on-demand, dynamic Tailscale integration in OpenBao. When a client (such as a Docker container) requests a secret from `docker/tailscale/auth-token/<name>` (for example, `docker/tailscale/auth-token/nginx`), OpenBao will automatically call the Tailscale API to generate a short-lived, preauthorized, and ephemeral authentication key labeled for that specific container, returning it to the client immediately.

---

## Architectural Choices & Routing

### Can we support other paths later (e.g., `docker/tailscale/idp`)?
**Yes, absolutely.** You have two clean options to achieve this:

1. **Option A: Extend the Plugin (Recommended)**
   Our plugin is modular. If you need a new path later, such as `docker/tailscale/idp` that interfaces with another service or maps configuration, you can simply add a new path handler inside the plugin code (e.g., creating `path_idp.go` and adding it to `Paths` in `backend.go`). This keeps all Tailscale-related integrations housed within a single secrets engine binary.

2. **Option B: OpenBao Nested Mount Routing**
   OpenBao (and HashiCorp Vault) routes API requests based on the **most specific matching path prefix**.
   * If you mount our plugin at `docker/tailscale`, it owns everything under `docker/tailscale/*`.
   * However, you can mount a completely separate secrets engine (like a standard Key-Value store or a database engine) at a nested subpath, such as `docker/tailscale/idp`:
     ```bash
     bao secrets enable -path=docker/tailscale/idp kv-v2
     ```
   In this scenario, a request to `docker/tailscale/idp/my-secret` is routed to the KV engine, while a request to `docker/tailscale/auth-token/nginx` is routed to our Tailscale plugin.

---

## Prerequisites
* OpenBao running in dev mode on the host (`127.0.0.1:8200` with root token `dev-only-token`).
* Docker installed on the host (used to compile the plugin without requiring a Go installation on your machine).
* A Tailscale API Key (e.g., `example-api-key-here`) and your Tailnet name.

---

## Step-by-Step Configuration

### Step 1: Compile the Plugin using Docker
Since Go is not required on your host, you can compile the plugin binaries for macOS and Linux using a containerized Go environment.

1. Navigate to the plugin source folder:
   ```bash
   cd wip/openbao-tailscale-integration
   ```

2. Build the Docker builder image:
   ```bash
   docker build -t openbao-tailscale-builder .
   ```

3. Run the builder to extract the binaries into a `bin/` directory on your host:
   ```bash
   docker run --rm -v "$(pwd)/bin:/out" openbao-tailscale-builder
   ```

This creates four binaries in your `wip/openbao-tailscale-integration/bin/` folder:
* `openbao-plugin-secrets-tailscale-darwin-arm64` (Apple Silicon Mac)
* `openbao-plugin-secrets-tailscale-darwin-amd64` (Intel Mac)
* `openbao-plugin-secrets-tailscale-linux-arm64` (Linux ARM64/Raspberry Pi)
* `openbao-plugin-secrets-tailscale-linux-amd64` (Linux AMD64)

### Step 2: Configure OpenBao for Plugins
To load external plugins, OpenBao must be configured with a plugin directory. 

If running OpenBao in **Dev Mode** on your macOS host, start the server using the `-dev-plugin-dir` flag and point it to the absolute path of your built binaries:

```bash
# Create a dedicated plugin directory
mkdir -p /Users/sam/Workspace/openbao-plugins

# Copy the appropriate binary to your plugins directory
# For Apple Silicon macOS:
cp bin/openbao-plugin-secrets-tailscale-darwin-arm64 /Users/sam/Workspace/openbao-plugins/openbao-plugin-secrets-tailscale
chmod +x /Users/sam/Workspace/openbao-plugins/openbao-plugin-secrets-tailscale

# Start OpenBao dev server with the plugin folder loaded
bao server -dev -dev-root-token-id="dev-only-token" -dev-plugin-dir="/Users/sam/Workspace/openbao-plugins"
```

### Step 3: Register the Plugin in the Catalog
Before enabling the secrets engine, OpenBao requires you to register the binary in its internal catalog with a SHA256 signature to guarantee code integrity.

1. Set your environment variables:
   ```bash
   export BAO_ADDR="http://127.0.0.1:8200"
   export BAO_TOKEN="dev-only-token"
   ```

2. Calculate the SHA256 checksum of your plugin binary:
   ```bash
   # On macOS:
   export PLUGIN_SHA=$(shasum -a 256 /Users/sam/Workspace/openbao-plugins/openbao-plugin-secrets-tailscale | cut -d' ' -f1)
   ```

3. Register the plugin into the OpenBao Catalog:
   ```bash
   bao plugin register \
     -sha256="${PLUGIN_SHA}" \
     secret \
     openbao-plugin-secrets-tailscale
   ```

### Step 4: Enable the Secrets Engine at `docker/tailscale`
Mount the newly registered secrets engine plugin:

```bash
bao secrets enable \
  -path=docker/tailscale \
  -plugin-name=openbao-plugin-secrets-tailscale \
  plugin
```

### Step 5: Configure the Tailscale Secrets Engine
Write your Tailscale credentials to the plugin's configuration endpoint:

```bash
bao write docker/tailscale/config \
  api_key="example-api-key-here" \
  tailnet="your-tailnet-name"
```

---

## Verification

To verify that the integration is working and generating dynamic tokens, request a read operation for a specific app name (e.g. `nginx`):

```bash
bao read docker/tailscale/auth-token/nginx
```

**Expected Output:**
```text
Key             Value
---             -----
auth_token      tskey-auth-kXXXXXX-XXXXXX
description     OpenBao dynamic token for nginx
expires         2026-06-07T16:30:00Z
key_id          key_XXXXXXXX
```

### Passing Custom Key Parameters
You can customize the characteristics of the generated Tailscale key at request time:

```bash
bao read docker/tailscale/auth-token/nginx \
  reusable=false \
  ephemeral=true \
  preauthorized=true \
  tags="tag:docker,tag:prod" \
  expiry_seconds=1800
```

---

## Requesting Secrets inside Docker Containers

When requesting secrets from within a Docker container, the container needs to resolve the host address `127.0.0.1:8200` to contact OpenBao.

### Common Pitfall: Localhost Resolution
Inside a Docker container, `127.0.0.1` refers to the container itself, not the host machine. 
* On macOS, Docker provides the host alias `host.docker.internal`.
* On Linux, you should use the default bridge gateway IP (typically `172.17.0.1`) or run Docker Compose with `extra_hosts` to inject the host IP.

### Example: Container Bootstrap Script (`entrypoint.sh`)
This script can run as the entrypoint inside your docker container (in this case, for an `nginx` service) to automatically register itself on Tailscale using the token fetched from OpenBao:

```bash
#!/bin/sh
set -e

# Define the service name
SERVICE_NAME="nginx"

echo "Fetching Tailscale auth token from OpenBao for ${SERVICE_NAME}..."

# Fetch the dynamic auth token using curl
BAO_RESPONSE=$(curl -s \
  --header "X-Bao-Token: dev-only-token" \
  http://host.docker.internal:8200/v1/docker/tailscale/auth-token/${SERVICE_NAME})

# Extract the token value from the JSON response
TAILSCALE_AUTH_KEY=$(echo "$BAO_RESPONSE" | grep -o '"auth_token":"[^"]*' | grep -o '[^"]*$')

if [ -z "$TAILSCALE_AUTH_KEY" ]; then
  echo "Error: Failed to retrieve Tailscale auth key from OpenBao."
  exit 1
fi

echo "Successfully retrieved auth token. Starting Tailscale..."

# Authenticate and bring up Tailscale node in the container
tailscaled --state=mem: &
sleep 2
tailscale up --authkey="${TAILSCALE_AUTH_KEY}" --accept-dns=false

echo "Tailscale is connected!"
exec "$@"
```

### Example: Docker Compose configuration
```yaml
version: '3.8'

services:
  app-node:
    image: alpine:latest
    container_name: tailscale_nginx
    cap_add:
      - NET_ADMIN
    devices:
      - /dev/net/tun:/dev/net/tun
    volumes:
      - ./entrypoint.sh:/entrypoint.sh
    entrypoint: ["/bin/sh", "/entrypoint.sh"]
    command: ["ping", "google.com"]
    extra_hosts:
      - "host.docker.internal:host-gateway" # Resolves the host OpenBao address on Linux
```

---

## Troubleshooting & Common Pitfalls

1. **Permission Denied (Execution)**: If OpenBao logs `fork/exec /Users/sam/Workspace/openbao-plugins/openbao-plugin-secrets-tailscale: permission denied`, verify that the plugin file is executable by running `chmod +x` on it.
2. **Architecture Mismatch**: Ensure you copied the correct architecture binary. If OpenBao runs on Apple Silicon macOS, you must use the `darwin-arm64` binary. If it runs inside a Linux Docker container, you must use the `linux-amd64` or `linux-arm64` binary depending on your host.
3. **Invalid Tag Prefix**: When requesting a token with tags, Tailscale API strictly requires tags to begin with `tag:` (e.g. `tag:docker`, not `docker`).

{% endraw %}