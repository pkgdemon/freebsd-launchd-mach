#!/bin/sh
# One-shot importer for Apple's launchd source.
#
# Clones apple-oss-distributions/launchd at the launchd-842.1.4 tag,
# strips the .git directory, and lands the tree under src/ at the repo
# root. The Mach-only files listed in plan §6.1 are NOT removed here —
# that is a separate follow-up commit so the verbatim Apple import is
# preserved as a clean baseline in the git history.
#
# Re-running is refused if src/ already exists and is non-empty; remove
# it explicitly first if you really want to redo the import.

set -eu

UPSTREAM_URL="https://github.com/apple-oss-distributions/launchd.git"
UPSTREAM_TAG="launchd-842.1.4"

script_dir=$(cd -- "$(dirname -- "$0")" && pwd)
repo_root=$(cd -- "$script_dir/.." && pwd)
dest="$repo_root/src"

if [ -e "$dest" ] && [ -n "$(ls -A "$dest" 2>/dev/null || true)" ]; then
	echo "import-source: $dest already exists and is non-empty; refusing to clobber." >&2
	echo "import-source: remove it first if you really mean to re-import." >&2
	exit 1
fi

if ! command -v git >/dev/null 2>&1; then
	echo "import-source: git not found in PATH." >&2
	exit 1
fi

echo "import-source: cloning $UPSTREAM_URL @ $UPSTREAM_TAG into $dest"
rm -rf "$dest"
git clone --depth 1 --branch "$UPSTREAM_TAG" "$UPSTREAM_URL" "$dest"

# Capture the upstream commit before nuking .git so we can stamp it.
upstream_sha=$(git -C "$dest" rev-parse HEAD)

rm -rf "$dest/.git"

echo "import-source: imported $UPSTREAM_TAG ($upstream_sha)"
echo "import-source: tree is at $dest"
echo "import-source: next step — commit the verbatim import, then run the §6.1 amputation as a separate commit."
