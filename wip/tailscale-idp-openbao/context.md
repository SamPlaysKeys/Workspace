# Context: Tailscale IDP for OpenBao

**Goal**: Explore integration methods to allow OpenBao to authenticate clients (both humans and machines/containers) using their Tailscale Identity, removing the need for static tokens or passwords.

**Current State**: 
We investigated the official Tailscale experimental project `tsidp` (Tailscale Identity Provider) and community alternative `tsiam` (Workload Identity over Tailscale). We've established three concrete integration paths:

1. **Human SSO (OpenBao OIDC + `tsidp`)**: Configuring OpenBao's native `JWT/OIDC` auth method to use `tsidp` as the Identity Provider.
2. **Machine/Container Login (OpenBao JWT + `tsiam`)**: Using `tsiam` to issue node-level OIDC JWTs that are validated by OpenBao's JWT auth method.
3. **Passwordless Node Auth (OpenBao custom auth plugin + Tailscale Local WHOIS)**: A custom OpenBao credential backend that verifies the caller's IP address (`RemoteAddr`) directly against the local tailscaled WHOIS API.

**Key Findings**:
* `tsidp` supports standard OIDC flows, JWT ID Tokens, and JWKS endpoints, making it directly compatible with OpenBao's native JWT/OIDC authentication engine.
* `tsidp` supports OAuth STS (Security Token Service) token exchange, which can be enabled via the `TSIDP_ENABLE_STS=1` environment variable.
* For machine identities, `tsiam` is specifically tailored to issue signed OIDC JWTs for nodes, which can be exchanged for OpenBao client tokens.
* The OpenBao/Vault plugin SDK exposes `Connection.RemoteAddr` (the client's IP and port) inside the `logical.Request` object, enabling a local WHOIS verification plugin.

**Open Questions**:
* Which model (JWT OIDC token exchange vs. Local IP WHOIS lookup) fits your homelab workload orchestration best?
* How does the container get access to the tailscaled Unix socket if we choose the local WHOIS approach?

**Key Constraints**:
* OpenBao runs in dev mode at `127.0.0.1:8200`.
* The solution should avoid managing static credentials inside Docker containers.
