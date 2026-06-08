---
type: Type
_sidebar_label: ADR
---

# ADR: Use Cloud Models for Inference over Local Models

## Status
**Accepted**

## Context
We are designing a locally hosted AI assistant deployed via Docker. A fundamental architectural decision is where the Large Language Model (LLM) inference will take place. 

The two primary options are:
1. **Local Inference**: Running models locally using engines like Ollama, vLLM, or llama.cpp.
2. **Cloud Inference**: Utilizing third-party APIs (e.g., OpenAI, Anthropic, Google Gemini) while keeping the application frontend and database hosted locally.

## Decision
We will use **Cloud Models (APIs)** for the assistant's inference engine, rather than hosting models locally. The locally hosted Docker stack will act as the client/interface, managing user accounts, chat history, and API key configurations.

## Rationale
While a fully local stack offers maximum privacy, the hardware requirements for running frontier-level models (or even highly capable smaller models) are substantial. By offloading inference to the cloud, we decouple the application's hosting requirements from the AI's compute requirements.

### Advantages (Versatility & Capability)
* **Hardware Independence**: The local Docker stack becomes extremely lightweight. It can run on a basic VPS, a Raspberry Pi, or an older NAS without needing expensive GPUs or massive amounts of RAM.
* **Access to Frontier Models**: We gain immediate access to state-of-the-art models (e.g., GPT-4o, Claude 3.5 Sonnet, Gemini 1.5 Pro) which vastly outperform what can be run on consumer hardware.
* **Lower Maintenance**: No need to manage model weights, update inference engines, or troubleshoot CUDA/ROCm driver issues.
* **Lower Upfront Cost**: Avoids the need to purchase dedicated AI hardware (GPUs).

### Drawbacks (Data Control & Cost)
* **Data Privacy**: Prompts and context are sent to third-party providers. We lose the absolute data sovereignty that comes with local, air-gapped inference.
* **Recurring Costs**: We are subject to pay-as-you-go API costs based on token usage, which can scale up with heavy use.
* **Dependency**: Requires a constant internet connection; the assistant goes down if the ISP or the API provider experiences an outage.

## Consequences
* The chosen UI/Backend stack must robustly support multiple cloud APIs (OpenAI-compatible endpoints, Anthropic, etc.).
* We need a secure way to manage and inject API keys into the Docker environment (e.g., via `.env` files or Docker Secrets).
* If data privacy becomes a strict requirement for specific workloads in the future, we may need to revisit this decision or adopt a hybrid approach (routing sensitive queries to a smaller local model).