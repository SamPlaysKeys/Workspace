#!/usr/bin/env python3
"""Ensure every docs/guides/**/*.md file carries the standard frontmatter keys.

Modes:
  --staged   (pre-commit) fix staged guide files and re-stage them
  --all      check every guide file (add --fix to repair in place)

Only missing keys are added; existing values are never rewritten. Values may be
empty, but every key must be present so guides stay filterable.
"""
import os
import re
import subprocess
import sys
import datetime

ROOT = subprocess.run(["git", "rev-parse", "--show-toplevel"], capture_output=True,
                      text=True).stdout.strip() or os.getcwd()
GUIDES = "docs/guides"
TODAY = datetime.date.today().isoformat()
REQUIRED = ["type", "status", "system", "category", "title", "tags", "date", "related_to", "references"]
VALID_TYPES = {"Guide", "Reference", "Troubleshooting", "Design", "ADR", "Index"}
STOP = {"and", "the", "for", "with", "without", "md", "guide", "readme", "in", "of", "to", "on"}
SYSTEMS = {
    "ansible": "Ansible Automation Platform (AAP)",
    "bigfix": "HCL BigFix",
    "dev-environment": "Dev Environment",
    "github-actions": "GitHub Actions",
    "gpu": "NVIDIA GPU",
    "jamboard": "Google Jamboard (baracus)",
    "openbao": "OpenBao",
    "openshift": "OpenShift Container Platform",
    "openshift-virtualization": "OpenShift Virtualization",
}
FM_RE = re.compile(r"^---\n(.*?)\n---\n", re.S)


def q(s):
    return '"' + s.replace("\\", "\\\\").replace('"', '\\"') + '"'


def git(*args):
    return subprocess.run(["git", *args], cwd=ROOT, capture_output=True, text=True).stdout


def first_commit_date(rel):
    out = git("log", "--follow", "--format=%as", "--", rel).split()
    return out[-1] if out else TODAY


def defaults(rel, body, present):
    parts = os.path.relpath(rel, GUIDES).split(os.sep)
    fn = parts[-1]
    top = parts[0] if len(parts) > 1 else None
    out = []
    if "type" not in present:
        out.append("type: " + ("Index" if fn.lower() == "readme.md" else "Guide"))
    if "status" not in present:
        out.append("status: Active")
    if "system" not in present:
        out.append("system: " + SYSTEMS.get(top, "")) if top in SYSTEMS else out.append("system:")
    if "category" not in present:
        out.append("category: Guides")
    if "title" not in present:
        h1 = re.search(r"^#\s+(.+)$", body, re.M)
        title = h1.group(1).strip() if h1 else os.path.splitext(fn)[0].replace("-", " ").title()
        out.append("title: " + q(title))
    if "tags" not in present:
        toks = [p.lower() for p in parts[:-1]]
        toks += [t for t in re.split(r"[-_]", os.path.splitext(fn)[0].lower())
                 if len(t) > 2 and t not in STOP and not t.isdigit()]
        tags = list(dict.fromkeys(toks))
        out.append("tags: [" + ", ".join(tags) + "]")
    if "date" not in present:
        out.append("date: " + first_commit_date(rel))
    if "related_to" not in present:
        out.append("related_to: []")
    if "references" not in present:
        out.append("references: []")
    return out


def process(rel, fix):
    """Return (problems, new_text or None)."""
    path = os.path.join(ROOT, rel)
    text = open(path, encoding="utf-8").read()
    m = FM_RE.match(text)
    problems = []
    if m:
        fm, body = m.group(1), text[m.end():]
        present = set(re.findall(r"^([A-Za-z_]+):", fm, re.M))
    else:
        fm, body, present = None, text, set()
    add = defaults(rel, body, present)
    if add:
        problems.append("missing keys: " + ", ".join(a.split(":")[0] for a in add))
    tm = re.search(r"^type:\s*(.*?)\s*$", fm or "", re.M)
    if tm and tm.group(1) not in VALID_TYPES:
        problems.append(f"unknown type '{tm.group(1)}' (expected one of {sorted(VALID_TYPES)})")
    new = None
    if add and fix:
        if m:
            new = "---\n" + fm + "\n" + "\n".join(add) + "\n---\n" + body
        else:
            new = "---\n" + "\n".join(add) + "\n---\n\n" + body
    return problems, new


def main():
    args = sys.argv[1:]
    staged = "--staged" in args
    if staged:
        files = [f for f in git("diff", "--cached", "--name-only", "--diff-filter=ACM").splitlines()]
    elif "--all" in args:
        files = [os.path.join(dp, fn)[len(ROOT) + 1:] for dp, _, fns in os.walk(os.path.join(ROOT, GUIDES))
                 for fn in fns]
    else:
        print(__doc__)
        return 2
    files = sorted(f for f in files if f.startswith(GUIDES + "/") and f.endswith(".md"))
    fix = staged or "--fix" in args
    unstaged = set(git("diff", "--name-only").splitlines())
    failed = False
    for rel in files:
        problems, new = process(rel, fix)
        if not problems:
            continue
        print(f"{rel}: " + "; ".join(problems))
        if new is not None:
            if staged and rel in unstaged:
                print("  cannot auto-fix: file has unstaged changes; fix frontmatter and re-stage")
                failed = True
                continue
            open(os.path.join(ROOT, rel), "w", encoding="utf-8").write(new)
            if staged:
                subprocess.run(["git", "add", rel], cwd=ROOT)
                print("  added missing frontmatter keys and re-staged")
            else:
                print("  fixed")
        if any(p.startswith("unknown type") for p in problems):
            failed = True
        elif new is None:
            failed = True
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
