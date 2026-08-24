#!/usr/bin/env sh
# homelab-doc-kit bootstrap — scaffold a new, self-contained homelab doc set.
# POSIX sh. No dependencies beyond cp/find/sed (present on every base UNIX/macOS).
set -eu

KIT_DIR="$(cd "$(dirname "$0")" && pwd)"
TEMPLATES="$KIT_DIR/templates"

NAME=""
DIR="docs/homelab"
FORCE=0

REPO_URL="https://github.com/SamPlaysKeys/Workspace"
REPO_BRANCH="main"
REPO_KIT_PATH="artifacts/homelab-doc-kit"

# Download the kit's templates from the upstream repo if they're missing locally.
# Prefers git sparse-checkout (grabs only the folder); falls back to a repo tarball.
fetch_templates() {
  echo "WARNING: templates not found at '$TEMPLATES'." >&2
  echo "         Check your source install, or let me download them." >&2
  echo "Attempting to download '$REPO_KIT_PATH/templates' from $REPO_URL ($REPO_BRANCH)..." >&2

  tmp=$(mktemp -d)
  src=""
  if command -v git >/dev/null 2>&1; then
    if git clone --depth 1 --filter=blob:none --sparse "$REPO_URL" "$tmp/clone" >/dev/null 2>&1 &&
       git -C "$tmp/clone" sparse-checkout set "$REPO_KIT_PATH" >/dev/null 2>&1; then
      src="$tmp/clone/$REPO_KIT_PATH/templates"
    fi
  fi
  if [ -z "$src" ] || [ ! -d "$src" ]; then
    url="$REPO_URL/archive/refs/heads/$REPO_BRANCH.tar.gz"
    if command -v curl >/dev/null 2>&1; then
      if curl -sSLf "$url" | tar -xz -C "$tmp" 2>/dev/null; then
        src="$tmp/Workspace-$REPO_BRANCH/$REPO_KIT_PATH/templates"
      fi
    elif command -v wget >/dev/null 2>&1; then
      if wget -qO- "$url" | tar -xz -C "$tmp" 2>/dev/null; then
        src="$tmp/Workspace-$REPO_BRANCH/$REPO_KIT_PATH/templates"
      fi
    fi
  fi
  if [ -z "$src" ] || [ ! -d "$src" ]; then
    rm -rf "$tmp"
    echo "ERROR: failed to download templates. Check network, or install from $REPO_URL." >&2
    return 1
  fi
  cp -R "$src" "$TEMPLATES"
  rm -rf "$tmp"
  if [ -z "$(ls -A "$TEMPLATES" 2>/dev/null)" ]; then
    echo "ERROR: downloaded templates directory is empty." >&2
    return 1
  fi
  return 0
}

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
if [ -z "$DIR" ] || [ "$DIR" = "/" ] || [ "$DIR" = "." ] || [ "$DIR" = ".." ]; then
  echo "ERROR: --dir is unsafe ('$DIR'). Choose a real subdirectory." >&2
  exit 1
fi
if [ -L "$DIR" ]; then
  echo "ERROR: --dir '$DIR' is a symlink; refusing to follow it." >&2
  exit 1
fi

# slugify: lowercase, non-alnum -> dash, trim dashes (no extended regex)
SLUG=$(printf '%s' "$NAME" | tr '[:upper:]' '[:lower:]' | sed 's/[^a-z0-9]/-/g; s/--*/-/g; s/^-//; s/-$//')

if [ -e "$DIR" ] && [ "$FORCE" -ne 1 ]; then
  echo "ERROR: '$DIR' already exists. Use --force to overwrite." >&2
  exit 1
fi

# Validate templates exist before doing anything destructive.
if [ ! -d "$TEMPLATES" ]; then
  echo "WARNING: templates not found at '$TEMPLATES'." >&2
  echo "         Check your source install." >&2
  if [ ! -t 0 ]; then
    echo "ERROR: templates missing and no interactive session to confirm a download." >&2
    echo "       Install them from $REPO_URL, or run interactively." >&2
    exit 1
  fi
  printf "Would you like to fetch the templates automatically from GitHub? [y/N] " >&2
  read -r ans || ans=""
  case "$ans" in
    y|Y|yes|YES) ;;
    *) echo "Aborted. Install templates from $REPO_URL." >&2; exit 1;;
  esac
  if ! fetch_templates; then
    exit 1
  fi
fi

# Escape a string for safe use in a sed replacement (handles &, /, \).
esc_val() {
  printf '%s' "$1" | sed -e 's/\\/\\\\/g' -e 's/[/&]/\\&/g'
}

# Prompt before overwriting an existing target (interactive only).
confirm_overwrite() {
  if [ ! -t 0 ]; then
    return 0
  fi
  printf "'$DIR' exists and will be overwritten. Continue? [y/N] " >&2
  read -r ans || ans=""
  case "$ans" in
    y|Y|yes|YES) return 0;;
    *) echo "Aborted." >&2; exit 1;;
  esac
}

if [ -e "$DIR" ] && [ "$FORCE" -eq 1 ]; then
  confirm_overwrite
fi

NAME_ESC=$(esc_val "$NAME")
SLUG_ESC=$(esc_val "$SLUG")

echo "Scaffolding '$NAME' (slug: $SLUG) into '$DIR'..."

mkdir -p "$(dirname "$DIR")"
rm -rf "$DIR"
cp -R "$TEMPLATES" "$DIR"

# Substitute tokens in every copied markdown file.
# Portable: newline-delimited read (no bash-only 'read -d'). Filenames here are our
# own templates (no newlines), so this is safe under any POSIX sh. sed writes to a
# temp file then mv over the original (avoids non-portable 'sed -i' GNU/BSD split).
find "$DIR" -type f -name '*.md' | while IFS= read -r f; do
  sed -e "s/{{LAB_NAME}}/$NAME_ESC/g" -e "s/{{LAB_SLUG}}/$SLUG_ESC/g" "$f" > "$f.tmp"
  mv "$f.tmp" "$f"
done

echo "Done."
echo ""
echo "Next steps:"
echo "  1. Open $DIR/README.md"
echo "  2. Hand $DIR/prompts/goal.session.md to your AI assistant to draft the Ideal State"
echo "  3. Iterate: Achievable -> How to -> Fit"
echo "  4. Fill $DIR/overview/lab-map.template.md to visualize the lab"
