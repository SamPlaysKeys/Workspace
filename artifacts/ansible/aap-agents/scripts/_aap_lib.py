"""Shared utilities for AAP helper scripts.

Credentials are read from:
  ~/.config/aap/token    — personal access token
  ~/.config/aap/hostname — AAP hostname (e.g. aap.example.com)

Run scripts/aap-setup to initialise these files.

Cache:
  Fetched payloads are written to ~/.cache/aap/<job_id>-<kind>.json|txt.
  Pass force_refresh=True to bypass the cache.
"""

import json
import os
import re
import ssl
import sys
import urllib.error
import urllib.request

CACHE_DIR = os.path.expanduser("~/.cache/aap")


# ---------------------------------------------------------------------------
# Credentials
# ---------------------------------------------------------------------------

def get_credentials():
    """Return (token, host) from environment variables or ~/.config/aap/."""
    token = os.environ.get("AAP_TOKEN")
    host = os.environ.get("AAP_HOST") or os.environ.get("AAP_HOSTNAME")
    if token and host:
        return token.strip(), host.strip()

    token_file = os.path.expanduser("~/.config/aap/token")
    host_file  = os.path.expanduser("~/.config/aap/hostname")
    for path in (token_file, host_file):
        if not os.path.exists(path) or os.path.getsize(path) == 0:
            print(f"ERROR: {path} is missing or empty. Set AAP_TOKEN/AAP_HOST or run scripts/aap-setup.", file=sys.stderr)
            sys.exit(1)
    with open(token_file) as f:
        token = f.read().strip()
    with open(host_file) as f:
        host = f.read().strip()
    return token, host


# ---------------------------------------------------------------------------
# HTTP helpers
# ---------------------------------------------------------------------------

def _get_ssl_context():
    insecure = os.environ.get("AAP_INSECURE", "").lower() in ("1", "true", "yes")
    if insecure:
        ctx = ssl.create_default_context()
        ctx.check_hostname = False
        ctx.verify_mode = ssl.CERT_NONE
        return ctx
    return None


def _handle_http_error(e):
    if e.code == 401:
        print("ERROR: 401 Unauthorized — token expired. Run: scripts/aap-setup --refresh-token",
              file=sys.stderr)
    else:
        try:
            body = e.read().decode(errors="replace")
        except Exception:
            body = "(unreadable)"
        print(f"ERROR: HTTP {e.code}: {body}", file=sys.stderr)
    sys.exit(1)


def aap_get(path, token, host):
    """Fetch a JSON API endpoint. Returns parsed dict."""
    url = f"https://{host}{path}"
    req = urllib.request.Request(url, headers={
        "Authorization": f"Bearer {token}",
        "Content-Type": "application/json",
    })
    ctx = _get_ssl_context()
    try:
        with urllib.request.urlopen(req, context=ctx) as resp:
            return json.loads(resp.read())
    except urllib.error.HTTPError as e:
        _handle_http_error(e)


def fetch_text(path, token, host):
    """Fetch a plain-text endpoint (e.g. stdout?format=txt). Returns string."""
    url = f"https://{host}{path}"
    req = urllib.request.Request(url, headers={"Authorization": f"Bearer {token}"})
    ctx = _get_ssl_context()
    try:
        with urllib.request.urlopen(req, context=ctx) as resp:
            return resp.read().decode("utf-8", errors="replace")
    except urllib.error.HTTPError as e:
        _handle_http_error(e)


# ---------------------------------------------------------------------------
# Cache helpers
# ---------------------------------------------------------------------------

def cache_path(job_id, kind, ext="json"):
    """Return the cache file path for a given job ID and data kind."""
    os.makedirs(CACHE_DIR, exist_ok=True)
    return os.path.join(CACHE_DIR, f"{job_id}-{kind}.{ext}")


def load_cache(job_id, kind, ext="json"):
    """Return cached content or None if not present."""
    path = cache_path(job_id, kind, ext)
    if os.path.exists(path):
        with open(path) as f:
            return json.load(f) if ext == "json" else f.read()
    return None


def save_cache(job_id, kind, data, ext="json"):
    """Write data to cache. data is dict (json) or str (txt)."""
    path = cache_path(job_id, kind, ext)
    with open(path, "w") as f:
        if ext == "json":
            json.dump(data, f, indent=2)
        else:
            f.write(data)
    return path


def cached_aap_get(path, token, host, job_id, kind, force_refresh=False):
    """aap_get with read/write cache. Returns (data, from_cache)."""
    if not force_refresh:
        cached = load_cache(job_id, kind)
        if cached is not None:
            return cached, True
    data = aap_get(path, token, host)
    save_cache(job_id, kind, data)
    return data, False


def cached_fetch_text(path, token, host, job_id, kind, force_refresh=False):
    """fetch_text with read/write cache. Returns (text, from_cache)."""
    if not force_refresh:
        cached = load_cache(job_id, kind, ext="txt")
        if cached is not None:
            return cached, True
    text = fetch_text(path, token, host)
    saved = save_cache(job_id, kind, text, ext="txt")
    return text, False


# ---------------------------------------------------------------------------
# Job metadata convenience
# ---------------------------------------------------------------------------

def get_job_meta(job_id, token, host, force_refresh=False):
    """Fetch /api/v2/jobs/<id>/ with cache. Returns job dict."""
    data, from_cache = cached_aap_get(
        f"/api/v2/jobs/{job_id}/", token, host,
        job_id=job_id, kind="meta", force_refresh=force_refresh,
    )
    return data


def parse_cluster(job):
    """Extract cluster, target, or host identifier from job extra_vars."""
    try:
        ev = json.loads(job.get("extra_vars", "{}") or "{}")
    except (json.JSONDecodeError, TypeError):
        ev = {}
    for key in ("cluster_name", "target_host", "openshift_cluster_name", "cluster", "target", "hostname"):
        if key in ev and ev[key]:
            return str(ev[key])
    return "?"


STATUS_SYMBOL = {
    "successful": "✓",
    "failed":     "✗",
    "canceled":   "⊘",
    "running":    "▶",
    "error":      "!",
    "pending":    "…",
    "waiting":    "…",
}


# ---------------------------------------------------------------------------
# Security scanning
# ---------------------------------------------------------------------------

# detect-secrets covers JSON-style secrets, private keys, AWS keys, JWTs, and
# basic-auth URLs.  The supplement below catches the shell/Ansible key=value
# style that detect-secrets KeywordDetector misses (it requires quoted values).
_SUPPLEMENT_PATTERNS = [
    ("Shell credential", re.compile(
        r'(?i)\b(?:password|passwd|secret)\s*=\s*(?!\s)[^\s,;\'\"]{4,}'
    )),
]

_DS_SETTINGS = {
    "plugins_used": [
        {"name": "KeywordDetector"},
        {"name": "PrivateKeyDetector"},
        {"name": "AWSKeyDetector"},
        {"name": "JwtTokenDetector"},
        {"name": "BasicAuthDetector"},
    ],
    "filters_used": [
        {"path": "detect_secrets.filters.heuristic.is_templated_secret"},
        {"path": "detect_secrets.filters.heuristic.is_potential_uuid"},
        {"path": "detect_secrets.filters.heuristic.is_likely_id_string"},
    ],
}


def security_scan(text):
    """Scan text for lines that may contain secrets or credentials.

    Uses detect-secrets (https://github.com/Yelp/detect-secrets) with a
    targeted plugin set (no noisy entropy detectors) plus a small supplement
    for Ansible-style key=value credentials.

    Returns a list of (lineno, secret_type, line) tuples — one entry per
    unique (lineno, type) pair.  The raw line is included so callers can
    display it; redaction is the caller's responsibility.

    Raises ImportError with an install hint if detect-secrets is not installed.
    """
    try:
        from detect_secrets.settings import transient_settings
        from detect_secrets.core import scan as ds_scan
    except ImportError:
        raise ImportError(
            "detect-secrets is required for security scanning.\n"
            "Install with: pip install -r scripts/requirements.txt"
        )

    hits = []
    lines = text.splitlines()

    with transient_settings(_DS_SETTINGS):
        for lineno, line in enumerate(lines, start=1):
            seen_types = set()

            # Primary: detect-secrets
            for secret in ds_scan.scan_line(line):
                stype = secret.type
                if stype not in seen_types:
                    seen_types.add(stype)
                    hits.append((lineno, stype, line))

            # Supplement: shell/Ansible key=value
            for label, pattern in _SUPPLEMENT_PATTERNS:
                if label not in seen_types and pattern.search(line):
                    seen_types.add(label)
                    hits.append((lineno, label, line))

    return hits
