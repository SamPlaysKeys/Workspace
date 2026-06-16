---
type: Reference
---
# Checking OpenShift Certificates

This guide covers methods for checking the expiration dates of critical OpenShift certificates, including the API server and Ingress controller. While OpenShift typically auto-rotates most internal certificates, manually verifying them can be useful for troubleshooting and custom monitoring.

## Key Certificate Locations

Critical certificates are stored as Kubernetes Secrets across different namespaces:

| Component | Namespace | Secret Name |
|-----------|-----------|------------|
| API Server | `openshift-kube-apiserver` | `kube-apiserver-cert-key-pair` |
| Ingress (default) | `openshift-ingress` | `router-certs-default` |
| Controller Manager | `openshift-kube-controller-manager` | Various cert secrets |
| Scheduler | `openshift-kube-scheduler` | Various cert secrets |

## Quick Checks

Use these one-liners to manually inspect specific certificates.

### API Server Certificate

To check the expiration date directly:

```bash
oc get secret -n openshift-kube-apiserver kube-apiserver-cert-key-pair \
  -o jsonpath='{.data.tls\.crt}' | base64 -d | openssl x509 -enddate -noout
```

To view full certificate details including the validity period:

```bash
oc get secret -n openshift-kube-apiserver kube-apiserver-to-kubelet-signer \
  -o jsonpath='{.data.tls\.crt}' | base64 -d | openssl x509 -text -noout | grep -A2 "Validity"
```

### Default Ingress Controller Certificate

To check the expiration date:

```bash
oc get secret -n openshift-ingress router-certs-default \
  -o jsonpath='{.data.tls\.crt}' | base64 -d | openssl x509 -enddate -noout
```

## Automated Expiration Checks

When dealing with many certificates, automation is necessary. Here are several approaches for scripting certificate checks.

### Option 1: Multi-Namespace Checking

This script iterates over key namespaces and extracts the end date for all TLS secrets:

```bash
#!/bin/bash

NAMESPACES=("openshift-kube-apiserver" "openshift-ingress" "openshift-config-managed")

for ns in "${NAMESPACES[@]}"; do
    echo "======== $ns ========"
    oc get secret -n "$ns" -o json 2>/dev/null | \
      jq -r '.items[] | select(.type == "kubernetes.io/tls") | .metadata.name' | \
      while read secret; do
        echo -n "$secret: "
        oc get secret -n "$ns" "$secret" -o jsonpath='{.data.tls\.crt}' | \
          base64 -d | openssl x509 -enddate -noout | cut -d= -f2
    done
    echo ""
done
```

### Option 2: Comprehensive Warning Script (Recommended)

This script calculates the remaining days for each certificate in a namespace and provides warnings if they are close to expiring. It handles epoch time conversion for robust comparison.

```bash
#!/bin/bash

NAMESPACE="openshift-kube-apiserver"
DAYS_WARNING=30

oc get secret -n "$NAMESPACE" -o json | jq -r '.items[] | select(.type == "kubernetes.io/tls") | .metadata.name' | while read secret; do
    echo "=== Certificate: $secret ==="
    
    # Extract and decode certificate
    CERT=$(oc get secret -n "$NAMESPACE" "$secret" -o jsonpath='{.data.tls\.crt}' | base64 -d)
    
    # Get expiration date in seconds since epoch
    EXPDATE=$(echo "$CERT" | openssl x509 -enddate -noout | cut -d= -f2)
    EXPDATE_EPOCH=$(date -d "$EXPDATE" +%s 2>/dev/null || date -j -f "%b %d %T %Y %Z" "$EXPDATE" +%s)
    NOW=$(date +%s)
    DAYS_LEFT=$(( ($EXPDATE_EPOCH - $NOW) / 86400 ))
    
    # Display results
    echo "Expires: $EXPDATE"
    echo "Days remaining: $DAYS_LEFT"
    
    # Warning if expiring soon
    if [ $DAYS_LEFT -lt $DAYS_WARNING ]; then
        echo "⚠️  WARNING: Certificate expires in $DAYS_LEFT days!"
    elif [ $DAYS_LEFT -lt 0 ]; then
        echo "❌ CRITICAL: Certificate has expired!"
    else
        echo "✓ OK"
    fi
    echo ""
done
```

*Note: Save as `check-certs.sh`, run `chmod +x check-certs.sh`, and configure it as a cron job for regular monitoring.*

### Option 3: Quick One-Liners

If you prefer one-liners over a script, you can use `jq` to loop through secrets:

```bash
oc get secret -n openshift-kube-apiserver -o json | \
  jq -r '.items[] | select(.type == "kubernetes.io/tls") | .metadata.name' | \
  while read secret; do
    echo "=== $secret ==="
    oc get secret -n openshift-kube-apiserver "$secret" \
      -o jsonpath='{.data.tls\.crt}' | base64 -d | openssl x509 -enddate -noout
  done
```

## Built-In Monitoring

While scripts are helpful for point-in-time checks, OpenShift provides built-in mechanisms for monitoring:

1. **Prometheus Alerts**: OpenShift's default monitoring stack includes Prometheus alerts for certificate expiration.
2. **Web Console**: Check **Observe → Alerting** in the OpenShift web console for active certificate expiration warnings.
3. **CSR Approval**: For pending Certificate Signing Requests (often relevant during node addition or cert rotation), check status with `oc get csr -w` and approve manually if necessary using `oc adm certificate approve <csr-name>`.