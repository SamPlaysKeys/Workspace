# Running Discussion

## 2026-06-07 15:51
We explored the crossover between this integration and the official Tailscale experimental project `tsidp` (Tailscale Identity Provider) located at [tailscale/tsidp](https://github.com/tailscale/tsidp).

### Crossover Analysis

`tsidp` is an OIDC/OAuth 2.1 identity provider server that runs directly inside your tailnet. It publishes a standard OIDC discovery document (`/.well-known/openid-configuration`) and JWKS endpoint (`/.well-known/jwks.json`). 

OpenBao has a native `jwt` authentication method (`bao auth enable jwt`) that can validate OIDC tokens signed by any OIDC provider's JWKS.

We compared `tsidp` and `tsiam` (Workload Identity over Tailscale) for OpenBao authentication:

---

### Architecture Option 1: Native OIDC + `tsidp` (Human SSO Focus)
* **Flow**:
  1. A user attempts to login to OpenBao (`bao login -method=oidc`).
  2. OpenBao redirects the user's browser to the `tsidp` OIDC authorize endpoint.
  3. `tsidp` authenticates the user (via their tailnet identity/node IP) and returns an OIDC ID token (JWT) to OpenBao.
  4. OpenBao validates the token signature using the `tsidp` JWKS endpoint and logs the user in.
* **Crossover**: Direct compatibility. This is the cleanest way to set up passwordless Single Sign-On (SSO) for human operators accessing OpenBao's Web UI and CLI.
* **Non-Interactive Workloads**: `tsidp` has experimental STS (Security Token Service) support (`TSIDP_ENABLE_STS=1`) which supports OIDC Token Exchange (RFC 8693). This can be used for delegated access between backend services.

---

### Architecture Option 2: Native JWT + `tsiam` (Machine/Workload Focus)
* **Flow**:
  1. A client container on a Tailscale node needs to log into OpenBao.
  2. The client queries its local `tsiam` daemon (running on the host or as a sidecar) to get a signed node-identity JWT.
  3. `tsiam` contacts the host's `tailscaled` socket to cryptographically sign a node-identity JWT representing that specific node (containing claims like node name, tags, and user owner).
  4. The client presents this JWT to OpenBao: `bao write auth/jwt/login jwt="<JWT>"`.
  5. OpenBao validates the JWT signature against `tsiam`'s public JWKS endpoint (which is hosted on the tailnet) and logs the container in.
* **Crossover**: Very clean passwordless authentication for automated workloads. The containers do not need to store static client secrets or password tokens; they generate tokens on the fly from their local node identity.

---

### Architecture Option 3: Local WHOIS Validation (Custom Auth Plugin)
* **Flow**:
  1. A container connects to OpenBao and sends a login request: `bao write auth/tailscale/login`.
  2. The custom OpenBao authentication plugin inspects the incoming request's transport details and extracts the client IP address (`req.Connection.RemoteAddr`).
  3. The plugin dials the local host's `tailscaled.sock` and queries `/localapi/v0/whois?addr=<client-ip>`.
  4. The plugin verifies that the node has the necessary tags (e.g. `tag:prod-app`) and belongs to the correct tailnet.
  5. The plugin issues an OpenBao client token with policies mapped to those tags.
* **Crossover**: Zero credentials on the client side. The container simply says "log me in" and OpenBao checks who is calling by asking the network layer.

---

### Recommendation for Homelab Rebuild
* For **human operators**: Deploy `tsidp` on the tailnet and enable the standard OpenBao OIDC login method.
* For **Docker workloads**:
  * If you prefer native, zero-plugin OpenBao setups: Deploy `tsiam` on your hosts to mint OIDC tokens for containers, validating them using OpenBao's built-in JWT engine.
  * If you prefer zero-credential client requests: Build a custom OpenBao auth plugin that performs local WHOIS checks on client connections.
