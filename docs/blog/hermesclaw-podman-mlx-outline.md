---
type: Note
---

# Blog Outline: Hermesclaw on macOS — Podman + LM Studio + MLX

*Running a local AI agent stack with Apple-native ML acceleration instead of Docker and llama.cpp.*

---

## Outline

### 1. Title & Hook
- Working title: *"Hermesclaw on Apple Silicon: Podman, LM Studio, and MLX — No Docker Required"*
- Hook: Most local AI tutorials assume Docker + llama.cpp + GGUF. On Apple Silicon, there's a faster, more native stack: Podman (no daemon tax), LM Studio (slick GUI), and MLX (Apple's Metal-baked inference engine). Here's how to wire them together.

### 2. What Is Hermesclaw?
- A local AI agent combining two concepts:
  - **Hermes**: Persistent conversational assistant for brainstorming, research, documentation
  - **Claw** (CLI + AI): A terminal-based agent for task execution, similar to Claude CLI / Codex CLI
- Runs entirely on your Mac — no cloud API dependency
- Processes natural language requests, executes commands, generates files, searches the web
- Uses local LLM inference via LM Studio + MLX backend

### 3. Why This Stack? (The Pitch)

| Component | Typical Choice | This Stack | Why |
|---|---|---|---|
| Container runtime | Docker Desktop | **Podman** | Daemonless, rootless, free — no licensing audit |
| Inference engine | llama.cpp | **LM Studio** | GUI-native, model browser, built-in server |
| Model format | GGUF | **MLX** | Apple Silicon native — 2-3x faster than llama.cpp on M-series |
| macOS VM | Docker VM (HyperKit) | **Podman Machine** (QEMU/vfkit) | Lighter VM, systemd-native restarts |

### 4. Prerequisites
- macOS on Apple Silicon (M1/M2/M3/M4) — MLX requires Metal
- [Podman](https://podman.io) (`brew install podman`)
- [LM Studio](https://lmstudio.ai) (free download; supports MLX backend)
- An MLX-compatible model (e.g., Hermes 3, Llama 3.2, Qwen 2.5 MLX variants — available via LM Studio's model browser)
- Hermesclaw source/config files (from your agent project)

### 5. Part 1: LM Studio + MLX — The Inference Layer

#### 5a. Why MLX Over GGUF on macOS
- MLX is Apple's official ML framework — uses Metal GPU directly
- No CPU/GPU bridge overhead (llama.cpp goes through Metal via MoltenVK/ggml-metal)
- Benchmarks: Mistral 7B at ~45 tok/s (MLX) vs ~20 tok/s (llama.cpp Q4) on M3 Max
- Smaller memory footprint per model quantization
- Downside: fewer model quantizations available; community smaller than GGUF

#### 5b. Installing LM Studio
- Download, drag to Applications
- Model browser: search for MLX-tagged models (e.g., `hermes-3-llama-3.1-8b-mlx`)
- Download and load a model
- Enable local inference server (port 1234 by default)
- Key settings: GPU offload (always on for MLX), context length, prompt format

#### 5c. Testing the Server
```bash
curl http://localhost:1234/v1/chat/completions \
  -H "Content-Type: application/json" \
  -d '{ "messages": [{"role": "user", "content": "Hello"}], "model": "hermes-3-llama-3.1-8b-mlx" }'
```
- LM Studio exposes an OpenAI-compatible API — no adapter needed

#### 5d. LM Studio Tips for Hermesclaw
- Setting up multiple prompt presets for different agent modes
- Managing context window for long conversations
- Loading larger models + aggressive quantization when RAM is tight
- Using LM Studio's built-in token probability viewer for debugging

### 6. Part 2: Podman Setup — The Container Layer

#### 6a. Podman Machine Initialization
```bash
podman machine init --cpus 6 --memory 8192 --disk-size 50
podman machine start
alias docker=podman
```
- Resource tuning for inference: LM Studio runs on macOS host (Metal), Podman runs agent tools in the VM
- The agent container talks to LM Studio at `host.docker.internal:1234` (Podman equivalent: `host.containers.internal`)

#### 6b. Container Networking Bridge
- Hermesclaw in the container needs to reach LM Studio on the macOS host
- Podman's `--add-host` flag: `--add-host host.containers.internal:host-gateway`
- Or use `podman run --network=slirp4netns` with host loopback access

#### 6c. Building the Hermesclaw Image
- Containerfile example:
```dockerfile
FROM python:3.12-slim
WORKDIR /app
COPY requirements.txt .
RUN pip install -r requirements.txt
COPY . .
CMD ["python", "main.py"]
```
- Build: `podman build -t hermesclaw:latest .`

#### 6d. Running the Agent
```bash
podman run -d --name hermesclaw --replace \
  --add-host host.containers.internal:host-gateway \
  -v ./workspace:/app/workspace \
  -e LM_STUDIO_HOST=http://host.containers.internal:1234 \
  -e AGENT_MODE=hermes \
  hermesclaw:latest
```
- Two modes: `hermes` (Discord bot) and `claw` (terminal agent) — or combined

### 7. Part 3: The Architecture — How It All Fits Together

```
┌─────────────────────────────────────────────────────┐
│                   macOS Host (Apple Silicon)         │
│                                                      │
│  ┌──────────────────┐    ┌──────────────────────┐   │
│  │   LM Studio       │    │  Podman Machine VM    │   │
│  │   (MLX inference) │◄──►│  ┌────────────────┐   │   │
│  │   localhost:1234  │    │  │ hermesclaw      │   │   │
│  │                   │    │  │ container       │   │   │
│  │  Metal GPU ⚡      │    │  │                │   │   │
│  └──────────────────┘    │  │ tools: bash,     │   │   │
│                          │  │   git, file ops, │   │   │
│  ┌──────────────────┐    │  │   web search     │   │   │
│  │  Discord / iTerm  │◄──►│  └────────────────┘   │   │
│  └──────────────────┘    └──────────────────────┘   │
└─────────────────────────────────────────────────────┘
```
- **Key insight:** MLX inference stays on the host with direct Metal GPU access (no GPU passthrough complexity)
- Podman container handles tool execution in an isolated, rootless environment
- Container talks to LM Studio over the host network bridge

### 8. Why Podman Over Docker for This Setup

#### 8a. Daemonless Architecture
- Docker Desktop: dockerd always running, ~1GB VM memory idle
- Podman: no daemon; `podman machine` stops when idle, starts on demand
- For an always-on Hermes bot, systemd-native management via Quadlet

#### 8b. No Licensing Overhead
- LM Studio is free, Podman is free, MLX is open-source
- Entire stack is OSS — no per-seat fees, no audit triggers
- Vendor-independent: no Docker Inc., no Red Hat subscription required

#### 8c. Rootless by Default
- Hermesclaw running in rootless container: if the agent executes a destructive command, it can't escape the user namespace
- Docker Desktop requires explicit rootless mode configuration

### 9. Model Selection for MLX + Local Agents

| Model | Size | MLX Available? | Notes |
|---|---|---|---|
| Hermes 3 Llama 3.1 8B | 8B | Yes | Best-in-class for agent tasks; native function calling |
| Llama 3.2 3B MLX | 3B | Yes | Lightweight, fast; good for Claw (quick commands) |
| Qwen 2.5 Coder 7B MLX | 7B | Yes | Stronger for coding tasks |
| Mistral Nemo 12B MLX | 12B | Yes | Needs 16GB+ RAM; best reasoning |
| Phi-3 Medium MLX | 14B | Community | Good balance of speed and quality |

- Recommendation: Hermes 3 8B MLX as primary; fallback to Llama 3.2 3B for latency-sensitive Claw interactions

### 10. Performance Tuning

#### 10a. MLX Context Size
- Default: 4096 tokens — enough for single-turn agent tasks
- Increase to 8192 for Hermes mode (conversational with history)
- Memory cost: ~1GB per 4096 tokens at 8B model (MLX 4-bit)

#### 10b. LM Studio Server Settings
- Batch size: higher = faster generation, more VRAM
- MLX backend: Metal GPU utilization target 80-90%
- Thread count: match your performance cores (e.g., 6 for M3 Max)

#### 10c. Podman Resource Allocation
- CPU: 4-6 cores (leave 1-2 for LM Studio)
- Memory: 4-6GB (agent runs Python + tools, not inference)
- Disk: 20-50GB (for workspace, model downloads)

#### 10d. Latency Budget
- Cold start: LM Studio load time (~5-15s depending on model)
- First token: MLX rapid (~100-200ms on M-series)
- Generation: 30-50 tok/s for 7-8B models
- Total round-trip for agent action: 2-10 seconds (including tool calls)

### 11. Comparison: Docker + llama.cpp + GGUF

| Aspect | Docker + llama.cpp + GGUF | Podman + LM Studio + MLX |
|---|---|---|
| Initial setup | Install Docker Desktop, compile or download llama.cpp | Install Podman + LM Studio (both binaries) |
| GPU acceleration | MoltenVK (Metal via Vulkan translation layer) | Direct Metal (no translation layer) |
| Inference speed | Baseline | 2-3x faster on same hardware |
| Model selection | Largest (thousands of GGUF models) | Growing (MLX ecosystem expanding, but smaller) |
| Container runtime | Dockerd (daemon) | Podman (daemonless) |
| GUI for models | None (manual downloads) | LM Studio built-in model browser |
| Server setup | Manual (llama.cpp server CLI flags) | Built-in (toggle switch in LM Studio) |
| macOS VM overhead | Docker Desktop VM always running | Podman Machine on-demand |
| Commercial licensing | Paid for enterprise | Fully free |

### 12. Gotchas & Debugging

#### 12a. "localhost:1234 connection refused" from container
- Fix: use `host.containers.internal` instead of `localhost`
- Ensure LM Studio's server is listening on `0.0.0.0` (not `127.0.0.1`)

#### 12b. MLX model not loading
- Ensure model is MLX-format (check `.safetensors` + config.json, not `.gguf`)
- LM Studio's model browser filters for MLX by default

#### 12c. Podman volume permission errors
- Rootless container: files written by container are owned by `nobody`
- Fix: `podman unshare chown -R UID:GID ./workspace`
- Or use `--userns=keep-id` in `podman run`

#### 12d. Token limit issues
- MLX models have fixed context sizes; exceeding causes truncation or OOM
- Hermesclaw should implement sliding window or conversation summarization

#### 12e. Metal memory pressure
- LM Studio + other apps (Xcode, browser) compete for GPU memory
- Monitor with `asitop` or `Activity Monitor`; reduce model size if swapping

### 13. The Verdict
- **Podman + LM Studio + MLX is the superior local AI stack on Apple Silicon** — faster inference, no Docker license, no daemon overhead
- The trade-off: smaller model ecosystem (MLX vs GGUF) and slightly more manual networking configuration
- For a 2025/2026 Mac: this stack keeps inference on Metal (the fastest path) while isolating agent execution in a rootless container
- Hermesclaw benefits doubly: Hermes mode gets fast conversational inference; Claw mode gets sandboxed tool execution
- **Bottom line:** If you're on Apple Silicon and want the best local AI agent experience, this stack is the current sweet spot

### 14. Call to Action
- Download LM Studio, grab an MLX Hermes model, and try the setup
- Compare inference speed vs your previous GGUF/llama.cpp setup — the gap is significant
- Contribute MLX quantization scripts for your favorite model
- Open issue: what's missing from the MLX ecosystem that GGUF has?

---

## Suggested Sidebars

- **"MLX vs llama.cpp: A Benchmark"** — Real tok/s numbers across M1/M2/M3/M4
- **"What About Ollama?"** — Ollama now supports MLX backend; how it compares to LM Studio
- **"Running Without a Container"** — When you might skip Podman altogether and use virtualenv + LM Studio directly
- **"GPU Memory Accounting"** — How much RAM does each model size really consume with MLX?

## Target Audience
- macOS developers running local AI agents or coding assistants
- Apple Silicon users frustrated with Docker + llama.cpp performance
- Anyone exploring local inference as an alternative to cloud APIs
- Hermes/agent project builders looking for the optimal Apple-native stack

## Format
- Step-by-step tutorial + architectural guide, ~3000 words
- Architecture diagram (ASCII or image)
- Config file snippets (Podman run flags, LM Studio settings, Hermesclaw environment)
- Benchmark comparison table
