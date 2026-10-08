#!/usr/bin/env python3
"""
Stream real-time stdout from a running AAP job.

Attempts WebSocket first (same channel the web UI uses); falls back to
fast REST polling (1 s interval) of the job_events endpoint when the
WebSocket endpoint is unavailable (nginx not proxying ASGI, etc.).

Usage:
    scripts/aap-stream.py <job-id>           # replay from the beginning
    scripts/aap-stream.py <job-id> --tail    # start from current tail (new output only)

Auth:
    Reads token from ~/.config/aap/token and hostname from ~/.config/aap/hostname.

Exit codes:
    0  job completed successfully
    1  job failed, cancelled, or connection error
"""

import json
import os
import signal
import ssl
import sys
import time
import urllib.request
import urllib.error
import uuid

CONFIG_DIR = os.path.expanduser("~/.config/aap")

TERMINAL_STATUSES = {"successful", "failed", "error", "canceled"}
POLL_INTERVAL = 1.0   # seconds between REST event polls
PAGE_SIZE     = 200   # events per page


def _read(path):
    try:
        return open(path).read().strip()
    except FileNotFoundError:
        sys.exit(f"ERROR: {path} not found — run ./scripts/aap-setup")


def _get_credentials():
    token = os.environ.get("AAP_TOKEN")
    host = os.environ.get("AAP_HOST") or os.environ.get("AAP_HOSTNAME")
    if token and host:
        return host.strip(), token.strip()
    return _read(f"{CONFIG_DIR}/hostname"), _read(f"{CONFIG_DIR}/token")


def _get_ssl_config():
    insecure = os.environ.get("AAP_INSECURE", "").lower() in ("1", "true", "yes") or "--insecure" in sys.argv
    if insecure:
        ctx = ssl.create_default_context()
        ctx.check_hostname = False
        ctx.verify_mode = ssl.CERT_NONE
        return ctx, {"cert_reqs": ssl.CERT_NONE}
    return ssl.create_default_context(), {}


def _api(hostname, token, path):
    url = f"https://{hostname}{path}"
    req = urllib.request.Request(url, headers={"Authorization": f"Bearer {token}"})
    ctx, _ = _get_ssl_config()
    with urllib.request.urlopen(req, context=ctx, timeout=10) as r:
        return json.loads(r.read())


def _try_websocket(hostname, token, job_id):
    """Attempt WebSocket streaming. Returns False if unavailable."""
    try:
        import websocket
    except ImportError:
        return False

    csrf   = uuid.uuid4().hex
    url    = f"wss://{hostname}/websocket"
    result = [None]
    connected = [False]

    def on_open(ws):
        connected[0] = True
        ws.send(json.dumps({
            "groups": {"jobs": ["status_changed", "summary"], "job_events": [job_id]},
            "xrftoken": csrf,
        }))

    def on_message(ws, raw):
        try:
            msg = json.loads(raw)
        except json.JSONDecodeError:
            return
        if "accept" in msg:
            if not msg["accept"]:
                ws.close()
            return
        if "groups_joined" in msg:
            return
        if msg.get("error"):
            ws.close()
            return
        stdout = msg.get("stdout", "")
        if stdout:
            print(stdout if stdout.endswith("\n") else stdout + "\n",
                  end="", flush=True)
        status = msg.get("status")
        if status in TERMINAL_STATUSES:
            result[0] = status
            ws.close()

    def on_error(ws, err):
        pass  # will fall through to REST poller

    _, ssl_opts = _get_ssl_config()
    ws = websocket.WebSocketApp(
        url,
        header=[f"Authorization: Bearer {token}", f"Cookie: csrftoken={csrf}"],
        on_open=on_open, on_message=on_message, on_error=on_error,
    )
    # Short timeout: if handshake fails quickly we fall back to REST
    import threading
    t = threading.Thread(target=ws.run_forever, kwargs={"sslopt": ssl_opts})
    t.daemon = True
    t.start()
    t.join(timeout=5)

    if not connected[0]:
        return False  # handshake failed — use REST fallback

    # If connected, wait for job to finish
    while t.is_alive():
        t.join(timeout=1)

    return result[0]


def _latest_event_id(hostname, token, job_id):
    """Return the highest event ID currently in the job, or 0 if none."""
    path = (f"/api/v2/jobs/{job_id}/job_events/"
            f"?order_by=-id&page_size=1")
    try:
        data = _api(hostname, token, path)
        results = data.get("results", [])
        return results[0]["id"] if results else 0
    except (urllib.error.URLError, KeyError, json.JSONDecodeError):
        return 0


def _rest_stream(hostname, token, job_id, tail=False):
    """Poll job_events endpoint until job completes. Near-real-time at 1s interval."""
    last_id = _latest_event_id(hostname, token, job_id) if tail else 0
    last_status = None

    while True:
        try:
            # Fetch new events since last seen id
            path = (f"/api/v2/jobs/{job_id}/job_events/"
                    f"?id__gt={last_id}&order_by=id&page_size={PAGE_SIZE}")
            data = _api(hostname, token, path)

            for event in data.get("results", []):
                stdout = event.get("stdout", "")
                if stdout:
                    print(stdout if stdout.endswith("\n") else stdout + "\n",
                          end="", flush=True)
                last_id = max(last_id, event.get("id", 0))

            # Check job status
            job = _api(hostname, token, f"/api/v2/jobs/{job_id}/")
            last_status = job.get("status")
            if last_status in TERMINAL_STATUSES:
                # Drain any remaining events
                data = _api(hostname, token,
                            f"/api/v2/jobs/{job_id}/job_events/"
                            f"?id__gt={last_id}&order_by=id&page_size={PAGE_SIZE}")
                for event in data.get("results", []):
                    stdout = event.get("stdout", "")
                    if stdout:
                        print(stdout if stdout.endswith("\n") else stdout + "\n",
                              end="", flush=True)
                break

        except (urllib.error.URLError, KeyError, json.JSONDecodeError) as e:
            print(f"\nAPI error: {e}", file=sys.stderr)

        time.sleep(POLL_INTERVAL)

    return last_status


def main():
    import argparse
    parser = argparse.ArgumentParser(description="Stream AAP job output in real time.")
    parser.add_argument("job_id", type=int, help="AAP job ID")
    parser.add_argument("--tail", action="store_true",
                        help="Start from the current tail instead of replaying from the beginning")
    parser.add_argument("--insecure", action="store_true",
                        help="Disable SSL certificate verification")
    args = parser.parse_args()

    job_id   = args.job_id
    hostname, token = _get_credentials()

    signal.signal(signal.SIGINT, lambda *_: sys.exit(0))

    mode = "tail" if args.tail else "start"
    print(f"Streaming job {job_id} from {hostname} ({mode}) ...\n", file=sys.stderr)

    # Try WebSocket first; fall back to REST polling.
    # _try_websocket.__doc__ is always truthy (function has a docstring) so the
    # else branch was dead code — simplified to always attempt WebSocket first.
    result = _try_websocket(hostname, token, job_id)
    if result is False or result is None:
        reason = "unavailable" if result is False else "dropped mid-stream"
        print(f"WebSocket {reason} — falling back to REST polling\n",
              file=sys.stderr)
        result = _rest_stream(hostname, token, job_id, tail=args.tail)

    if result == "successful":
        print(f"\n✓ Job {job_id} completed successfully.", file=sys.stderr)
        sys.exit(0)
    elif result:
        print(f"\n✗ Job {job_id} ended: {result}", file=sys.stderr)
        sys.exit(1)
    else:
        print(f"\n✗ Job {job_id} stream ended without a terminal status (connection dropped?).",
              file=sys.stderr)
        sys.exit(1)


if __name__ == "__main__":
    main()

