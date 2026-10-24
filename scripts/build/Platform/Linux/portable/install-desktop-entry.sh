#!/usr/bin/env bash
#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

# Adds (or removes) desktop integration for a portable O3DE install for the
# current user only. This is the portable equivalent of what the .deb's postinst
# does system wide. No root required.

set -o errexit # exit on the first failure encountered
set -o nounset

ENGINE_ROOT=$(cd "$(dirname "$(readlink -f "${BASH_SOURCE[0]}")")" && pwd)
BIN_DIR=$ENGINE_ROOT/bin/Linux/profile/Default
VERSION=$(sed -n 's/^[[:space:]]*"display_version"[[:space:]]*:[[:space:]]*"\([^"]*\)".*/\1/p' "$ENGINE_ROOT/engine.json" | head -1)
if [[ -z "$VERSION" || "$VERSION" == "00.00" ]]; then
    VERSION=$(sed -n 's/^[[:space:]]*"version"[[:space:]]*:[[:space:]]*"\([^"]*\)".*/\1/p' "$ENGINE_ROOT/engine.json" | head -1)
fi

APPLICATIONS_DIR=${XDG_DATA_HOME:-$HOME/.local/share}/applications
DESKTOP_FILE=$APPLICATIONS_DIR/o3de-$VERSION.desktop
LOCAL_BIN=$HOME/.local/bin

usage() {
    cat <<EOF
Usage: $(basename "$0") [--links] [--uninstall]

  (no options)  Add an "O3DE $VERSION" entry to the application menu
  --links       Also link o3de, o3de.sh, o3de.editor and o3de.assetprocessor into $LOCAL_BIN
  --uninstall   Remove the menu entry and any links that point into this install
EOF
}

CREATE_LINKS=false
UNINSTALL=false
while [[ $# -gt 0 ]]; do
    case "$1" in
        --links)     CREATE_LINKS=true; shift ;;
        --uninstall) UNINSTALL=true; shift ;;
        -h|--help)   usage; exit 0 ;;
        *)           echo "Unknown option: $1" >&2; usage >&2; exit 1 ;;
    esac
done

declare -A LINKS=(
    [o3de]=$ENGINE_ROOT/o3de
    [o3de.sh]=$ENGINE_ROOT/scripts/o3de.sh
    [o3de.editor]=$BIN_DIR/Editor
    [o3de.assetprocessor]=$BIN_DIR/AssetProcessor
)

if [[ "$UNINSTALL" == true ]]; then
    rm -f "$DESKTOP_FILE"
    for name in "${!LINKS[@]}"; do
        # Only remove links that belong to this install
        if [[ -L "$LOCAL_BIN/$name" && "$(readlink "$LOCAL_BIN/$name")" == "${LINKS[$name]}" ]]; then
            rm -f "$LOCAL_BIN/$name"
        fi
    done
    command -v update-desktop-database &> /dev/null && update-desktop-database "$APPLICATIONS_DIR" || true
    echo "Removed O3DE $VERSION desktop integration"
    exit 0
fi

mkdir -p "$APPLICATIONS_DIR"
cat > "$DESKTOP_FILE" <<EOF
[Desktop Entry]
Version=1.0
Name=O3DE $VERSION
Comment=O3DE Project Manager
Type=Application
Exec="$ENGINE_ROOT/o3de"
Path=$BIN_DIR/
Icon=$ENGINE_ROOT/o3de_desktop.svg
Terminal=false
Categories=Development;Graphics;
StartupWMClass=o3de
StartupNotify=true
EOF
chmod 644 "$DESKTOP_FILE"
command -v update-desktop-database &> /dev/null && update-desktop-database "$APPLICATIONS_DIR" || true
echo "Added application menu entry: $DESKTOP_FILE"

if [[ "$CREATE_LINKS" == true ]]; then
    mkdir -p "$LOCAL_BIN"
    for name in "${!LINKS[@]}"; do
        ln -sf "${LINKS[$name]}" "$LOCAL_BIN/$name"
    done
    echo "Linked o3de, o3de.sh, o3de.editor and o3de.assetprocessor into $LOCAL_BIN"
    if [[ ":$PATH:" != *":$LOCAL_BIN:"* ]]; then
        echo "Note: $LOCAL_BIN is not on your PATH"
    fi
fi
