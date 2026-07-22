#!/usr/bin/env sh
# homelab-doc-kit bootstrap — scaffold a new, self-contained homelab doc set.
# POSIX sh. No dependencies beyond cp/find/sed (present on every base UNIX/macOS).
set -eu

KIT_DIR="$(cd "$(dirname "$0")" && pwd)"
TEMPLATES="$KIT_DIR/templates"

NAME=""
DIR="docs/homelab"
FORCE=0

usage() {
  cat <<'EOF'
homelab-doc-kit — scaffold a new homelab documentation set.

Usage:
  ./bootstrap.sh --name "My Homelab" [--dir docs/homelab] [--force]

Options:
  -n, --name NAME   Human-readable lab name (required)
  -d, --dir DIR     Target directory (default: docs/homelab)
  -f, --force       Overwrite target if it already exists
  -h, --help        Show this help

Example:
  ./bootstrap.sh --name "Smith Homelab" --dir docs/homelab
EOF
}

while [ $# -gt 0 ]; do
  case "$1" in
    -n|--name) NAME="$2"; shift 2;;
    -d|--dir) DIR="$2"; shift 2;;
    -f|--force) FORCE=1; shift;;
    -h|--help) usage; exit 0;;
    *) echo "Unknown arg: $1" >&2; usage >&2; exit 1;;
  esac
done

if [ -z "$NAME" ]; then
  echo "ERROR: --name is required." >&2
  usage >&2
  exit 1
fi
if [ -z "$DIR" ] || [ "$DIR" = "/" ]; then
  echo "ERROR: --dir is unsafe ('$DIR'). Choose a real subdirectory." >&2
  exit 1
fi

# slugify: lowercase, non-alnum -> dash, trim dashes (no extended regex)
SLUG=$(printf '%s' "$NAME" | tr '[:upper:]' '[:lower:]' | sed 's/[^a-z0-9]/-/g; s/--*/-/g; s/^-//; s/-$//')

if [ -e "$DIR" ] && [ "$FORCE" -ne 1 ]; then
  echo "ERROR: '$DIR' already exists. Use --force to overwrite." >&2
  exit 1
fi

echo "Scaffolding '$NAME' (slug: $SLUG) into '$DIR'..."

mkdir -p "$(dirname "$DIR")"
rm -rf "$DIR"
cp -R "$TEMPLATES" "$DIR"

# Substitute tokens in every copied markdown file.
# Portable: newline-delimited read (no bash-only 'read -d'). Filenames here are our
# own templates (no newlines), so this is safe under any POSIX sh. sed writes to a
# temp file then mv over the original (avoids non-portable 'sed -i' GNU/BSD split).
find "$DIR" -type f -name '*.md' | while IFS= read -r f; do
  sed -e "s/{{LAB_NAME}}/$NAME/g" -e "s/{{LAB_SLUG}}/$SLUG/g" "$f" > "$f.tmp"
  mv "$f.tmp" "$f"
done

echo "Done."
echo ""
echo "Next steps:"
echo "  1. Open $DIR/README.md"
echo "  2. Hand $DIR/prompts/goal.session.md to your AI assistant to draft the Ideal State"
echo "  3. Iterate: Achievable -> How to -> Fit"
echo "  4. Fill $DIR/overview/lab-map.template.md to visualize the lab"
