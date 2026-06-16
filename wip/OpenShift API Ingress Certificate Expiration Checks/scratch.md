
# Checking API and Ingress Certificates on OpenShift

Here are the main methods to verify certificates haven't expired:

## **1. Check API Server Certificate**

The API server certificate is typically stored in the `kube-apiserver` secret:

```bash
# View the certificate details
oc get secret -n openshift-kube-apiserver kube-apiserver-to-kubelet-signer \
  -o jsonpath='{.data.tls\.crt}' | base64 -d | openssl x509 -text -noout | grep -A2 "Validity"

# Or extract and check expiration directly
oc get secret -n openshift-kube-apiserver kube-apiserver-cert-key-pair \
  -o jsonpath='{.data.tls\.crt}' | base64 -d | openssl x509 -enddate -noout
```

## **2. Check Ingress Controller Certificate**

The default ingress controller certificate is in the `ingress-operator` namespace:

```bash
# Check the default ingress certificate
oc get secret -n openshift-ingress router-certs-default \
  -o jsonpath='{.data.tls\.crt}' | base64 -d | openssl x509 -enddate -noout

# View full certificate details
oc get secret -n openshift-ingress router-certs-default \
  -o jsonpath='{.data.tls\.crt}' | base64 -d | openssl x509 -text -noout
```

## **3. Check All Certificates at Once**

For a comprehensive view:

```bash
# List all certificates in key namespaces
oc get secret -n openshift-kube-apiserver -o wide
oc get secret -n openshift-ingress -o wide
oc get secret -n openshift-config-managed -o wide
```

## **4. Using OpenShift Certificate Management Tools**

```bash
# Check certificate status (OpenShift 4.5+)
oc adm certificate approve <csr-name>

# View certificate status
oc get csr -w
```

## **Key Locations to Monitor**

| Component | Namespace | Secret Name |
|-----------|-----------|------------|
| API Server | `openshift-kube-apiserver` | `kube-apiserver-cert-key-pair` |
| Ingress (default) | `openshift-ingress` | `router-certs-default` |
| Controller Manager | `openshift-kube-controller-manager` | Various cert secrets |
| Scheduler | `openshift-kube-scheduler` | Various cert secrets |

## **Automated Monitoring**

Consider setting up alerts:
- Use Prometheus to monitor certificate expiration
- Enable certificate rotation in OpenShift (automatic for most certs)
- Check the certificate status in the web console: **Observe → Alerting** for cert expiration warnings

The `notAfter` field in the certificate output shows the expiration date. OpenShift typically auto-rotates certificates, but it's good to monitor them regularly.


# Checking TLS Certificate Validity

Here are several approaches to automate this:

## **1. Simple One-Liner with Grep and xargs**

```bash
oc get secret -n openshift-kube-apiserver -o wide | grep kubernetes.io/tls | \
  awk '{print $1}' | xargs -I {} sh -c \
  'echo "=== {} ===" && oc get secret -n openshift-kube-apiserver {} \
  -o jsonpath="{.data.tls\.crt}" | base64 -d | openssl x509 -enddate -noout'
```

## **2. Using jsonpath for Better Filtering**

```bash
oc get secret -n openshift-kube-apiserver -o json | \
  jq -r '.items[] | select(.type == "kubernetes.io/tls") | .metadata.name' | \
  while read secret; do
    echo "=== $secret ==="
    oc get secret -n openshift-kube-apiserver "$secret" \
      -o jsonpath='{.data.tls\.crt}' | base64 -d | openssl x509 -enddate -noout
  done
```

## **3. Comprehensive Bash Script**

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

## **4. Check Multiple Namespaces**

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

## **5. JSON Output for Parsing**

```bash
oc get secret -n openshift-kube-apiserver -o json | \
  jq -r '.items[] | select(.type == "kubernetes.io/tls") | 
  .metadata.name as $name | 
  .data."tls.crt" | 
  @base64d | 
  . as $cert | 
  {name: $name, cert: $cert}' | \
  jq -Rs '@base64d' 2>/dev/null
```

## **Recommended Approach**

For production use, I'd recommend **Option 3** (the comprehensive script) because it:
- Shows expiration dates and days remaining
- Warns if certificates expire within 30 days (configurable)
- Works across different systems
- Easy to understand and modify

Simply save as `check-certs.sh`, make executable with `chmod +x check-certs.sh`, and run it regularly via cron for automated monitoring.

