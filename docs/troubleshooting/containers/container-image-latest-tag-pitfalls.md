---
type: Note
---
# Container image tag `latest` vs a real version tag

This guide explains why using the tag **`latest`** as the **only** tag (or as a stand-in for “whatever we built most recently”) causes operational and security problems, how that shows up on Kubernetes and OpenShift, and how to remediate. It is **not** arguing that the string `latest` is forbidden everywhere; the issue is **using `latest` instead of an immutable, meaningful identifier** for production and shared infrastructure.

---

## Symptoms

Problems often appear **indirectly** because registries allow the **`latest`** tag to **move** to a new image digest when someone pushes again.

| Symptom | Why it happens |
|--------|----------------|
| **“We pushed a fix but the cluster still runs the old bits.”** | With **`imagePullPolicy: IfNotPresent`** (the default when the image tag is **not** `latest`), the kubelet **does not** re-pull if the image is already on the node—even if **`latest`** in the registry now points at a **new** digest. See [Kubernetes: Images — Updating images](https://kubernetes.io/docs/concepts/containers/images/#updating-images). |
| **Different nodes run different code for the “same” `image:repo:latest`.** | Each node caches images locally. Unless every node re-pulls at the same time (and resolves the same digest), you can get **version skew**. Kubernetes explicitly warns that if a registry **retargets** a tag, you can end up with a **mix of old and new** pods; see [Kubernetes: Images — Image pull policy](https://kubernetes.io/docs/concepts/containers/images/#image-pull-policy). |
| **Rollbacks are unclear or impossible.** | “Roll back to `latest`” is not a version. You lack a stable tag or digest that pointed to the previously known-good artifact. |
| **Support and audits cannot answer “what build is running?”** | `kubectl get pod -o wide` shows `:latest`, not a release or digest. SBOM, CVE triage, and vendor support all want a **pin**. |
| **Operators / Helm / CRs reference `…:latest` and drift silently.** | Many controllers do **not** re-resolve tags on every reconcile unless the spec changes or pull policy forces a pull. |
| **“ImagePullBackOff” after deleting `latest` or renaming tags.** | Consumers still reference a tag that no longer exists or is no longer readable. |

---

## Explanation

### Tags and digests (OCI)

A **container image** in a registry is addressed by:

- A **tag** (a mutable string such as `latest` or `v1.4.2`), and/or  
- A **digest** (an immutable content hash, e.g. `sha256:…`).

The **OCI Distribution Specification** defines how registries store and address content; tags are **not** guaranteed to be stable pointers. See the [OCI Distribution Specification](https://github.com/opencontainers/distribution-spec/blob/master/spec.md) and the [OCI Image Specification — digests](https://github.com/opencontainers/image-spec/blob/main/descriptor.md#digests).

Kubernetes summarizes the difference clearly: **tags can move**; **digests identify one exact image**. See [Kubernetes: Images](https://kubernetes.io/docs/concepts/containers/images/).

### What `latest` means in practice

`latest` is **just another tag**. It has **no special “newest version” behavior** in the registry API. Conventionally, tools default to `latest` when you omit a tag (for example `docker pull nginx` pulls `nginx:latest`), which trains people to treat it as “the build,” but that is a **convention**, not a contract.

### How Kubernetes chooses pull behavior

From [Kubernetes: Images — Image pull policy](https://kubernetes.io/docs/concepts/containers/images/#image-pull-policy):

- If you omit `imagePullPolicy` and the tag is **`:latest`**, Kubernetes sets **`imagePullPolicy` to `Always`**.  
- If you omit `imagePullPolicy` and the tag is **anything other than `latest`**, the default is **`IfNotPresent`**.

Important corollaries from the same page:

- **`Always`** still resolves the name to a **digest** at pull time; if the digest matches the node cache, layers are not re-downloaded—but you **do** consult the registry for resolution.  
- **`IfNotPresent`** can **skip** pulls entirely when the tag is already cached, which is why **`latest` + IfNotPresent** (for example after changing a Deployment from a pinned tag to `latest` without updating pull policy) is a footgun; see [Default image pull policy](https://kubernetes.io/docs/concepts/containers/images/#default-image-pull-policy).  
- The **`imagePullPolicy` is set when the object is first created** and is **not** automatically updated when you later change only the image tag; you may need to **edit pull policy explicitly** after such a change.

Kubernetes **recommends avoiding `:latest` in production** and using a meaningful tag and/or digest; see [Kubernetes: Images — Updating images](https://kubernetes.io/docs/concepts/containers/images/#updating-images).

### Registries (Docker Hub, Quay, ECR, integrated OpenShift registry)

Registries **store tags as pointers to manifests**. Pushing `myrepo/app:latest` again typically **moves** `latest` to a new digest; old digests may become **untagged** but still exist until garbage collection, depending on policy. For **Red Hat Quay**, routine operations include **viewing and modifying tags** in the UI; see [Use Red Hat Quay — viewing and modifying tags](https://docs.redhat.com/en/documentation/red_hat_quay/3.14/html-single/use_red_hat_quay/index#viewing-and-modifying-tags). Quay also supports **tag expiration** and **Time Machine**-style retention at the org level (see organization settings in the same guide).

---

## Troubleshooting

1. **Confirm what the workload references**

   ```bash
   kubectl get pod <pod> -o jsonpath='{.spec.containers[*].image}{"\n"}'
   ```

   On OpenShift:

   ```bash
   oc get pod <pod> -n <namespace> -o jsonpath='{.spec.containers[*].image}{"\n"}'
   ```

2. **Inspect effective pull policy**

   ```bash
   kubectl get pod <pod> -o jsonpath='{.spec.containers[*].imagePullPolicy}{"\n"}'
   ```

3. **Compare registry digest vs node**

   Use `skopeo inspect` (or the registry UI) to read the digest for `repo:tag`, then compare to what the node is running (container runtime / `crictl images` on the node, or cluster-specific tooling). If **`latest`** moved but **`IfNotPresent`** prevented a re-pull, the node can lag the registry.

4. **Check for tag-only references in Git / Helm / Operators**

   Search CI and manifests for `:latest` and for image fields that omit a tag (which implies `latest` on some runtimes). Pin in **source control**, not only in the live cluster.

5. **Quay / registry UI**

   Open the repository’s **Tags** page and verify whether **`latest`** is the only tag, when it last moved, and whether version tags exist. For Quay, see [viewing and modifying tags](https://docs.redhat.com/en/documentation/red_hat_quay/3.14/html-single/use_red_hat_quay/index#viewing-and-modifying-tags).

---

## Remediation

### 1. Add a real version tag (keep `latest` temporarily if needed)

Build or retag the **same image** (same digest) with a meaningful tag, for example:

- Application semver: `v2.3.1`  
- Vendor artifact: `9.0.0.0`, `510.73.06-rhcos4.15`, etc.  
- CI build id: `git-abc1234` or pipeline run number  

Push:

```bash
podman tag myregistry/org/app:latest myregistry/org/app:v2.3.1
podman push myregistry/org/app:v2.3.1
```

### 2. Point workloads at the pinned tag (or digest)

Update Deployments, StatefulSets, CronJobs, **ClusterPolicy**, **Subscription** catsrc images, MTV VDDK init image fields, etc., to **`…:v2.3.1`** or to **`image@sha256:…`**. Digests give the strongest immutability guarantee; see [Kubernetes: Images](https://kubernetes.io/docs/concepts/containers/images/).

### 3. Force a rollout if the tag string did not change

If you **must** repoint a tag to new content (discouraged), bump something that forces new pods: change an **annotation**, use **`kubectl rollout restart deployment/…`**, or temporarily set **`imagePullPolicy: Always`** (understand the registry load and supply-chain implications).

### 4. Fix mistaken `imagePullPolicy` after tag changes

If you changed a Deployment from `app:1.2.3` to `app:latest`, the policy may still be **`IfNotPresent`**; Kubernetes will **not** auto-flip it to **`Always`**. See [Kubernetes: Default image pull policy](https://kubernetes.io/docs/concepts/containers/images/#default-image-pull-policy). Align policy with how you want upgrades to behave.

### 5. Retire `latest` for production paths

After all consumers use pinned tags or digests:

- Remove **`latest`** from the registry (Quay UI, or `skopeo delete docker://registry/org/repo:latest`) so CI cannot accidentally depend on it again.

Order matters: **update consumers first**, then delete **`latest`**, or you will cause **`ImagePullBackOff`**.

---

## Prevention

| Practice | Reference |
|----------|-----------|
| Require semver or digest pins in CI for deployable artifacts | Team policy + lint (`grep ':latest'`) |
| Prefer **`image@sha256:`** for highest immutability | [Kubernetes: Images](https://kubernetes.io/docs/concepts/containers/images/) |
| Use admission policy (OPA/Gatekeeper/Kyverno) to reject `:latest` in prod namespaces | [Kubernetes: admission controllers](https://kubernetes.io/docs/reference/access-authn-authz/admission-controllers/) |
| For cached-image security, understand **`AlwaysPullImages`** and (where relevant) credential verification behavior | [AlwaysPullImages admission controller](https://kubernetes.io/docs/reference/access-authn-authz/admission-controllers/#alwayspullimages), [Kubernetes images doc](https://kubernetes.io/docs/concepts/containers/images/) |
| Document “how we tag releases” for first-party and third-party images | Internal runbook |

---

## When `latest` can be acceptable

- **Local developer loops** where you intentionally want “whatever I built last” and you control cache and teardown.  
- **Short-lived CI** jobs that pull once per job with **`Always`** or always-fresh runners.  

Even then, **release artifacts** handed to other teams or clusters should use **real tags or digests**.

---

## Related workspace notes

- [Using the integrated OpenShift container image registry](../../guides/openshift/using-integrated-openshift-registry.md) — pushing with explicit tags to the integrated registry.  
- [NVIDIA vGPU Manager: integrated registry and ClusterPolicy](../../guides/openshift/nvidia-vgpu-manager-internal-registry-and-clusterpolicy.md) — example of pinning operator-managed images.

---

## References

1. [Kubernetes — Images](https://kubernetes.io/docs/concepts/containers/images/) (tags vs digests, pull policy, avoid `latest` in production).  
2. [Kubernetes — Image pull policy](https://kubernetes.io/docs/concepts/containers/images/#image-pull-policy) and [Default image pull policy](https://kubernetes.io/docs/concepts/containers/images/#default-image-pull-policy).  
3. [Kubernetes — Updating images](https://kubernetes.io/docs/concepts/containers/images/#updating-images).  
4. [OCI Distribution Specification](https://github.com/opencontainers/distribution-spec/blob/master/spec.md).  
5. [OCI Image Specification — digests](https://github.com/opencontainers/image-spec/blob/main/descriptor.md#digests).  
6. [Red Hat Quay — Use Red Hat Quay](https://docs.redhat.com/en/documentation/red_hat_quay/3.14/html-single/use_red_hat_quay/index) (repository and tag operations; [viewing and modifying tags](https://docs.redhat.com/en/documentation/red_hat_quay/3.14/html-single/use_red_hat_quay/index#viewing-and-modifying-tags)).  
7. [Docker — docker tag](https://docs.docker.com/reference/cli/docker/image/tag/) (tagging and pushing semantics in the Docker CLI model).  
8. [Podman — podman-tag](https://docs.podman.io/en/latest/markdown/podman-tag.1.html).
