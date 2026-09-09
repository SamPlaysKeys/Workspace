#!/usr/bin/env python3
"""
AAP Inventory & Job Template Exporter (Python Proof-of-Concept)

Fetches Projects, Inventories, Execution Environments, Job Templates,
and Workflow Job Templates from Ansible Automation Platform (AAP) /
Automation Controller v2 API, enriches them with human-readable names,
and outputs unified CSV or JSON reports.
"""

import argparse
import csv
import json
import os
import ssl
import sys
import urllib.error
import urllib.parse
import urllib.request


def parse_args():
    parser = argparse.ArgumentParser(
        description="Export and enrich AAP / AWX Job Templates and Workflow Job Templates."
    )
    parser.add_argument(
        "--host",
        default=os.getenv("CONTROLLER_HOST") or os.getenv("AAP_HOST"),
        help="AAP Controller host URL (e.g. https://controller.example.com)",
    )
    parser.add_argument(
        "--token",
        default=os.getenv("CONTROLLER_TOKEN") or os.getenv("AAP_TOKEN"),
        help="AAP API OAuth2 Bearer token",
    )
    parser.add_argument(
        "-k",
        "--insecure",
        action="store_true",
        help="Disable SSL verification for self-signed certificates",
    )
    parser.add_argument(
        "-f",
        "--format",
        choices=["csv", "json"],
        default="csv",
        help="Output format (default: csv)",
    )
    parser.add_argument(
        "-o",
        "--output",
        help="Output file path (default: stdout)",
    )
    return parser.parse_args()


def get_ssl_context(insecure: bool) -> ssl.SSLContext:
    if insecure:
        ctx = ssl.create_default_context()
        ctx.check_hostname = False
        ctx.verify_mode = ssl.CERT_NONE
        return ctx
    return ssl.create_default_context()


def fetch_all_results(url: str, token: str, ssl_ctx: ssl.SSLContext) -> list:
    results = []
    current_url = url

    headers = {
        "Authorization": f"Bearer {token}",
        "Accept": "application/json",
        "User-Agent": "AAP-Inventory-Exporter/1.0",
    }

    while current_url:
        req = urllib.request.Request(current_url, headers=headers)
        try:
            with urllib.request.urlopen(req, context=ssl_ctx) as response:
                if response.status != 200:
                    sys.stderr.write(f"Error fetching {current_url}: HTTP {response.status}\n")
                    sys.exit(1)
                data = json.loads(response.read().decode("utf-8"))
                if "results" in data:
                    results.extend(data["results"])
                current_url = data.get("next")
                if current_url and not current_url.startswith("http"):
                    base = urllib.parse.urlparse(url)
                    current_url = f"{base.scheme}://{base.netloc}{current_url}"
        except urllib.error.HTTPError as e:
            if e.code == 404:
                sys.stderr.write(f"Endpoint not found (HTTP 404): {current_url}, skipping...\n")
                return results
            sys.stderr.write(f"HTTP Error fetching {current_url}: {e.code} {e.reason}\n")
            sys.exit(1)
        except urllib.error.URLError as e:
            sys.stderr.write(f"URL Error fetching {current_url}: {e.reason}\n")
            sys.exit(1)

    return results


def main():
    args = parse_args()

    if not args.host or not args.token:
        sys.stderr.write("Error: Both --host and --token (or AAP_HOST / AAP_TOKEN env vars) are required.\n")
        sys.exit(1)

    host = args.host.rstrip("/")
    if not host.startswith("http://") and not host.startswith("https://"):
        host = f"https://{host}"

    ssl_ctx = get_ssl_context(args.insecure)

    # 1. Fetch Projects lookup map
    sys.stderr.write("Fetching Projects...\n")
    projects_raw = fetch_all_results(f"{host}/api/v2/projects/?page_size=100", args.token, ssl_ctx)
    project_map = {p["id"]: p.get("name", "") for p in projects_raw}

    # 2. Fetch Inventories lookup map
    sys.stderr.write("Fetching Inventories...\n")
    inventories_raw = fetch_all_results(f"{host}/api/v2/inventories/?page_size=100", args.token, ssl_ctx)
    inventory_map = {i["id"]: i.get("name", "") for i in inventories_raw}

    # 3. Fetch Execution Environments lookup map
    sys.stderr.write("Fetching Execution Environments...\n")
    ee_raw = fetch_all_results(f"{host}/api/v2/execution_environments/?page_size=100", args.token, ssl_ctx)
    ee_map = {e["id"]: e.get("name", "") for e in ee_raw}

    # 4. Fetch Job Templates (standard)
    sys.stderr.write("Fetching Job Templates...\n")
    job_templates_raw = fetch_all_results(f"{host}/api/v2/job_templates/?page_size=100", args.token, ssl_ctx)

    # 5. Fetch Workflow Job Templates
    sys.stderr.write("Fetching Workflow Job Templates...\n")
    workflow_templates_raw = fetch_all_results(
        f"{host}/api/v2/workflow_job_templates/?page_size=100", args.token, ssl_ctx
    )

    enriched = []

    # Process standard Job Templates
    for jt in job_templates_raw:
        proj_id = jt.get("project")
        inv_id = jt.get("inventory")
        ee_id = jt.get("execution_environment")

        item = {
            "id": jt.get("id"),
            "name": jt.get("name"),
            "type": "job_template",
            "job_type": jt.get("job_type", "run"),
            "project_id": proj_id,
            "project_name": project_map.get(proj_id, "") if proj_id else "",
            "inventory_id": inv_id,
            "inventory_name": inventory_map.get(inv_id, "") if inv_id else "",
            "execution_environment_id": ee_id,
            "execution_environment_name": ee_map.get(ee_id, "") if ee_id else "",
            "playbook": jt.get("playbook", ""),
            "forks": jt.get("forks", 0),
            "verbosity": jt.get("verbosity", 0),
        }
        enriched.append(item)

    # Process Workflow Job Templates
    for wf in workflow_templates_raw:
        inv_id = wf.get("inventory")
        item = {
            "id": wf.get("id"),
            "name": wf.get("name"),
            "type": "workflow",
            "job_type": "N/A",
            "project_id": None,
            "project_name": "N/A",
            "inventory_id": inv_id,
            "inventory_name": inventory_map.get(inv_id, "") if inv_id else "N/A",
            "execution_environment_id": None,
            "execution_environment_name": "N/A",
            "playbook": "N/A",
            "forks": "N/A",
            "verbosity": "N/A",
        }
        enriched.append(item)

    # Output formatting
    output_stream = open(args.output, "w", encoding="utf-8") if args.output else sys.stdout

    try:
        if args.format == "json":
            json.dump(enriched, output_stream, indent=2)
            output_stream.write("\n")
        else:
            fieldnames = [
                "id",
                "name",
                "type",
                "job_type",
                "project_id",
                "project_name",
                "inventory_id",
                "inventory_name",
                "execution_environment_id",
                "execution_environment_name",
                "playbook",
                "forks",
                "verbosity",
            ]
            writer = csv.DictWriter(output_stream, fieldnames=fieldnames)
            writer.writeheader()
            writer.writerows(enriched)
    finally:
        if args.output:
            output_stream.close()

    sys.stderr.write(
        f"Done! Exported {len(job_templates_raw)} job templates and {len(workflow_templates_raw)} workflows (Total: {len(enriched)}).\n"
    )


if __name__ == "__main__":
    main()
