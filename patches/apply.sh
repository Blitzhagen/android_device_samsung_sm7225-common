#!/usr/bin/env bash
#
# Applies the sm7225-common framework patches to the ROM source tree.
# Run once after `repo sync`, from anywhere:
#
#   device/samsung/sm7225-common/patches/apply.sh
#
# Patch files live in subdirectories named after the target repository
# (e.g. patches/frameworks/base/*.patch -> frameworks/base/). They are
# applied in filename order with `git am`, preserving author and message.
# To revert:  cd <repo> && git reset --hard m/<branch>

set -euo pipefail

PATCHES_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(cd "$PATCHES_DIR/../../../.." && pwd)"

find "$PATCHES_DIR" -type d | sort | while read -r dir; do
    ls "$dir"/*.patch >/dev/null 2>&1 || continue
    repo="${dir#"$PATCHES_DIR"/}"
    echo ">>> $repo"
    for p in "$dir"/*.patch; do
        (cd "$ROOT/$repo" && git am --whitespace=fix "$p")
    done
done
