#!/usr/bin/env python3
import os
import sys
import shutil
import re
import yaml

def parse_markdown_file(file_path):
    """
    Parses a markdown file and returns its frontmatter (dict) and body (str).
    """
    try:
        with open(file_path, "r", encoding="utf-8") as f:
            content = f.read()
    except Exception as e:
        print(f"Error reading {file_path}: {e}")
        return {}, ""

    if content.startswith("---"):
        parts = content.split("---", 2)
        if len(parts) >= 3:
            frontmatter_text = parts[1]
            body = parts[2]
            try:
                frontmatter = yaml.safe_load(frontmatter_text) or {}
                return frontmatter, body
            except Exception as e:
                print(f"Error parsing YAML in {file_path}: {e}")
                return {}, content
    return {}, content

def write_markdown_file(file_path, frontmatter, body):
    """
    Writes a markdown file with the updated frontmatter and body.
    """
    try:
        os.makedirs(os.path.dirname(file_path), exist_ok=True)
        # safe_dump preserves unicode and formats structures cleanly
        frontmatter_text = yaml.safe_dump(frontmatter, sort_keys=False, allow_unicode=True)
        with open(file_path, "w", encoding="utf-8") as f:
            f.write(f"---\n{frontmatter_text}---\n{body}")
    except Exception as e:
        print(f"Error writing to {file_path}: {e}")

def get_h1_title(body):
    """
    Scans the markdown body to extract the first H1 header as the title.
    """
    for line in body.splitlines():
        match = re.match(r"^#\s+(.+)$", line.strip())
        if match:
            return match.group(1).strip()
    return None

def main():
    if len(sys.argv) < 3:
        print("Usage: sync_pages.py <source_docs_dir> <target_pages_dir>")
        sys.exit(1)

    source_dir = os.path.abspath(sys.argv[1])
    target_dir = os.path.abspath(sys.argv[2])

    print(f"Syncing markdown from '{source_dir}' to '{target_dir}'...")

    synced_files = set()  # Set of all relative file paths written/updated in this run

    # We will walk the source docs directory recursively
    for root, dirs, files in os.walk(source_dir):
        for file in files:
            file_path = os.path.join(root, file)
            rel_path = os.path.relpath(file_path, source_dir)
            rel_parts = rel_path.split(os.sep)

            # Check if this is a markdown file
            if file.endswith(".md"):
                # Parse frontmatter and body
                frontmatter, body = parse_markdown_file(file_path)

                # 1. Check for explicit exclusion/draft flags in frontmatter
                is_ignored = (
                    frontmatter.get("draft") is True or
                    frontmatter.get("publish") is False or
                    frontmatter.get("exclude") is True or
                    frontmatter.get("pages") == "exclude" or
                    frontmatter.get("sync") is False
                )
                if is_ignored:
                    print(f"Skipping {rel_path} (explicitly excluded/draft)")
                    continue

                # 2. Determine default category based on directory name
                first_dir = rel_parts[0] if len(rel_parts) > 1 else ""
                is_homelab = first_dir.lower() == "homelab"

                category = None
                status = None

                if is_homelab:
                    # Homelab doesn't require frontmatter fields, has defaults
                    category = frontmatter.get("category", "Homelab")
                    status = frontmatter.get("status", "Active")
                else:
                    # Strict gatekeeper: Must have BOTH category and status
                    category = frontmatter.get("category")
                    status = frontmatter.get("status")

                    if not category or not status:
                        # Skip files outside homelab that lack both category and status
                        continue

                # 3. Clean and normalize category and status
                category = str(category).strip()
                status = str(status).strip()

                # 4. Extract or determine Title
                title = frontmatter.get("title")
                if not title:
                    title = get_h1_title(body)
                if not title:
                    # Fallback to filename capitalized
                    base_name = os.path.splitext(file)[0]
                    title = base_name.replace("-", " ").replace("_", " ").title()
                else:
                    title = str(title).strip()

                # 5. Build target relative path for GH_Pages
                category_kebab = category.lower().replace(" ", "-").replace("_", "-")
                first_dir_normalized = first_dir.lower().replace(" ", "-").replace("_", "-")

                if first_dir_normalized == category_kebab:
                    # Preserving original path structure (e.g. docs/guides -> guides/)
                    target_subpath = rel_parts[1:]
                else:
                    target_subpath = rel_parts[1:] if len(rel_parts) > 1 else rel_parts

                # Deduplicate the folder name if the subpath already starts with the category kebab
                if target_subpath and target_subpath[0].lower().replace(" ", "-").replace("_", "-") == category_kebab:
                    target_subpath = target_subpath[1:]

                target_rel_path = os.path.join(category_kebab, *target_subpath)

                # Ensure layout is page
                final_frontmatter = frontmatter.copy()
                final_frontmatter["layout"] = "page"
                final_frontmatter["title"] = title
                final_frontmatter["category"] = category
                final_frontmatter["status"] = status

                target_file_path = os.path.join(target_dir, target_rel_path)
                write_markdown_file(target_file_path, final_frontmatter, body)
                print(f"Synced: {rel_path} -> {target_rel_path}")
                synced_files.add(target_rel_path)

            else:
                # Support copying assets (png, jpg, pdf, html, etc.)
                # Determine its default category folder based on parent dir
                first_dir = rel_parts[0] if len(rel_parts) > 1 else ""
                if first_dir:
                    category_kebab = first_dir.lower().replace(" ", "-").replace("_", "-")
                    # Target relative path preserves the subdirectories
                    target_subpath = rel_parts[1:]
                    target_rel_path = os.path.join(category_kebab, *target_subpath)
                    
                    target_file_path = os.path.join(target_dir, target_rel_path)
                    os.makedirs(os.path.dirname(target_file_path), exist_ok=True)
                    shutil.copy2(file_path, target_file_path)
                    print(f"Asset Copied: {rel_path} -> {target_rel_path}")
                    synced_files.add(target_rel_path)

    # 6. Cleanup obsolete files on GH_Pages
    print("Performing obsolete files cleanup on target directory...")
    # Scan target directory recursively for any .md or assets that we did NOT write in this run
    for root, dirs, files in os.walk(target_dir):
        # Skip hidden files/directories (like .git, .github) and templates
        if any(part.startswith(".") for part in os.path.relpath(root, target_dir).split(os.sep)):
            continue
        
        for file in files:
            file_path = os.path.join(root, file)
            rel_path = os.path.relpath(file_path, target_dir)

            # Do not delete index.md, _config.yml, README.md or standard Jekyll templates
            if rel_path in ["index.md", "_config.yml", "README.md"]:
                continue

            if rel_path not in synced_files:
                print(f"Deleting obsolete file: {rel_path}")
                os.remove(file_path)
                
                # Clean up empty parent directories up to target_dir
                parent_dir = os.path.dirname(file_path)
                while parent_dir != target_dir:
                    try:
                        if not os.listdir(parent_dir):
                            print(f"Removing empty directory: {os.path.relpath(parent_dir, target_dir)}")
                            os.rmdir(parent_dir)
                            parent_dir = os.path.dirname(parent_dir)
                        else:
                            break
                    except Exception:
                        break

    print("Sync complete!")

if __name__ == "__main__":
    main()
