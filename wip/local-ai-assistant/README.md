# Nemoclaw Deployment

Multi-channel AI assistant gateway deployed via Docker with NVIDIA GPU passthrough on Fedora.

## Architecture

This compose spins up two services from the [OpenClaw](https://github.com/openclaw/openclaw) source (built as the `nemoclaw` image):

- **`nemoclaw-gateway`** — The core gateway process exposing ports 18789 (Control UI) and 18790 (bridge)
- **`nemoclaw-cli`** — A CLI companion sharing the gateway's network namespace for interactive use

## Prerequisites

### 1. NVIDIA Container Toolkit

```bash
sudo dnf install -y nvidia-container-toolkit
sudo nvidia-ctk runtime configure --runtime=docker
sudo systemctl restart docker
```

Verify the GPU is visible to Docker:

```bash
docker run --rm --runtime=nvidia nvidia/cuda:12-base nvidia-smi
```

### 2. Host Directories

The compose expects these directories on the host:

```
/DATA/AppData/Nemoclaw/
├── openclaw/       → mounted as /home/node/.openclaw (config, state, secrets)
└── workspace/      → mounted as /home/node/.openclaw/workspace (agent workspace)
```

Create them before first run:

```bash
sudo mkdir -p /DATA/AppData/Nemoclaw/{openclaw,workspace}
```

### 3. Docker Buildx

Building from a git URL requires BuildKit (default in modern Docker). Verify:

```bash
docker buildx version
```

## Deploy

```bash
docker compose -f wip/local-ai-assistant/docker-compose.yml up -d --build
```

The first build pulls the full OpenClaw repo and compiles it — this takes a few minutes.

## Post-Deploy

### 1. Run onboarding

```bash
docker compose -f wip/local-ai-assistant/docker-compose.yml exec nemoclaw-cli openclaw onboard
```

### 2. Open the Control UI

Browse to `http://<vm-ip>:18789/`

### 3. Connect channels

See https://docs.openclaw.ai/channels for channel-specific setup (Discord, Telegram, WhatsApp, etc.)

## Environment Variables

| Variable | Default | Description |
|---|---|---|
| `OPENCLAW_GATEWAY_TOKEN` | (none) | Auth token for gateway access |
| `OPENCLAW_GATEWAY_PORT` | 18789 | Control UI port |
| `OPENCLAW_BRIDGE_PORT` | 18790 | Bridge port |
| `OPENCLAW_TZ` | UTC | Timezone |
| `OPENCLAW_ALLOW_INSECURE_PRIVATE_WS` | (none) | Allow insecure WebSocket connections |

## Updating

```bash
docker compose -f wip/local-ai-assistant/docker-compose.yml build --no-cache
docker compose -f wip/local-ai-assistant/docker-compose.yml up -d
```

## Notes

- The gateway binds to `0.0.0.0` (`--bind lan`) for external access. Set `OPENCLAW_GATEWAY_TOKEN` to secure it.
- The `OPENCLAW_INSTALL_DOCKER_CLI=1` build arg enables sandbox isolation for agent code execution (requires `/var/run/docker.sock` to be mounted — uncomment in the compose if needed).
- GPU passthrough uses the NVIDIA Container Toolkit runtime. The GPU is available inside the container for companion services (Ollama, local models, etc.).
