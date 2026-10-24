#!/usr/bin/env bash
#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

# Creates a portable (no package manager) O3DE tarball from either an installed
# engine directory (e.g. /opt/O3DE/26.10.0 after installing the .deb) or a .deb file.
# The tarball contents match what the .deb installs, with the post-install steps
# (bundled CMake extraction, build leftovers cleanup) already applied.

set -o errexit # exit on the first failure encountered
set -o nounset
set -o pipefail

SCRIPT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)

usage() {
    cat <<EOF
Usage: $(basename "$0") [options] <source>

  <source>          An installed engine directory (e.g. /opt/O3DE/26.10.0) or an O3DE .deb file

Options:
  -o, --output DIR  Directory to write the tarball to (default: ./dist)
  -n, --name NAME   Base name of the tarball (default: o3de_<version>_linux_<arch>)
  -z, --gzip        Compress with gzip (.tar.gz) instead of xz (.tar.xz)
  -s, --sign KEYID  Create a detached, armored GPG signature (.asc) with KEYID
  -k, --keep        Keep the staging directory after the tarball is created
  -h, --help        Show this help
EOF
}

OUTPUT_DIR=$PWD/dist
PACKAGE_NAME=
COMPRESSION=xz
SIGN_KEY=
KEEP_STAGING=false
SOURCE=

while [[ $# -gt 0 ]]; do
    case "$1" in
        -o|--output) OUTPUT_DIR=$2; shift 2 ;;
        -n|--name)   PACKAGE_NAME=$2; shift 2 ;;
        -z|--gzip)   COMPRESSION=gzip; shift ;;
        -s|--sign)   SIGN_KEY=$2; shift 2 ;;
        -k|--keep)   KEEP_STAGING=true; shift ;;
        -h|--help)   usage; exit 0 ;;
        -*)          echo "Unknown option: $1" >&2; usage >&2; exit 1 ;;
        *)           SOURCE=$1; shift ;;
    esac
done

if [[ -z "$SOURCE" ]]; then
    usage >&2
    exit 1
fi

mkdir -p "$OUTPUT_DIR"
OUTPUT_DIR=$(cd "$OUTPUT_DIR" && pwd)
STAGING_ROOT=$(mktemp -d "$OUTPUT_DIR/.staging.XXXXXX")
cleanup() {
    if [[ "$KEEP_STAGING" != true ]]; then
        rm -rf "$STAGING_ROOT"
    fi
}
trap cleanup EXIT

# 1. Get the engine files into the staging directory
if [[ -d "$SOURCE" ]]; then
    ENGINE_SOURCE=$(cd "$SOURCE" && pwd)
    echo "[portable] Copying installed engine from $ENGINE_SOURCE"
elif [[ -f "$SOURCE" && "$SOURCE" == *.deb ]]; then
    echo "[portable] Extracting $SOURCE"
    DEB_ROOT=$STAGING_ROOT/deb
    mkdir -p "$DEB_ROOT"
    if command -v dpkg-deb &> /dev/null; then
        dpkg-deb -x "$SOURCE" "$DEB_ROOT"
    else
        # Without dpkg (e.g. Fedora/Arch): a .deb is an ar archive holding data.tar.*
        DEB_SOURCE=$(cd "$(dirname "$SOURCE")" && pwd)/$(basename "$SOURCE")
        (cd "$DEB_ROOT" && ar x "$DEB_SOURCE" && tar -xf data.tar.* && rm -f data.tar.* control.tar.* debian-binary _gpg*)
    fi
    ENGINE_JSON=$(find "$DEB_ROOT" -maxdepth 4 -name engine.json -print -quit)
    if [[ -z "$ENGINE_JSON" ]]; then
        echo "[portable] Error: no engine.json found in $SOURCE" >&2
        exit 1
    fi
    ENGINE_SOURCE=$(dirname "$ENGINE_JSON")
else
    echo "[portable] Error: $SOURCE is neither a directory nor a .deb file" >&2
    exit 1
fi

if [[ ! -f "$ENGINE_SOURCE/engine.json" ]]; then
    echo "[portable] Error: $ENGINE_SOURCE does not contain engine.json" >&2
    exit 1
fi

# Use the display version (what the .deb is named after), falling back to the engine version
read_engine_field() {
    sed -n "s/^[[:space:]]*\"$1\"[[:space:]]*:[[:space:]]*\"\([^\"]*\)\".*/\1/p" "$ENGINE_SOURCE/engine.json" | head -1
}
VERSION=$(read_engine_field display_version)
if [[ -z "$VERSION" || "$VERSION" == "00.00" ]]; then
    VERSION=$(read_engine_field version)
fi
ARCH=$(uname -m)
TOP_DIR=o3de_${VERSION}
PACKAGE_NAME=${PACKAGE_NAME:-o3de_${VERSION}_linux_${ARCH}}
STAGE=$STAGING_ROOT/$TOP_DIR

echo "[portable] Engine version $VERSION, staging to $STAGE"
if [[ -n "${DEB_ROOT:-}" ]]; then
    mv "$ENGINE_SOURCE" "$STAGE"
    rm -rf "$DEB_ROOT"
else
    cp -a "$ENGINE_SOURCE" "$STAGE"
fi

# 2. Apply the post-install steps from cmake/Platform/Linux/Packaging/postinst.in
if [[ ! -x "$STAGE/cmake/runtime/bin/cmake" ]]; then
    CMAKE_ARCHIVE=$(find "$STAGE/Tools/Redistributables/CMake" -maxdepth 1 -name 'cmake-*-linux-*.tar.gz' -print -quit 2>/dev/null || true)
    if [[ -z "$CMAKE_ARCHIVE" ]]; then
        echo "[portable] Error: bundled CMake not found in Tools/Redistributables/CMake" >&2
        exit 1
    fi
    echo "[portable] Extracting bundled CMake into cmake/runtime"
    mkdir -p "$STAGE/cmake/runtime"
    tar -xzf "$CMAKE_ARCHIVE" -C "$STAGE/cmake/runtime" --strip-components=1
fi

# Leftovers from the installer build or from running the engine (see prerm.in)
echo "[portable] Removing build and runtime leftovers"
find "$STAGE" \( -name '*.egg-info' -o -name __pycache__ \) -prune -exec rm -rf {} +
find "$STAGE/bin/Linux" -mindepth 3 -maxdepth 3 -type d -name user -prune -exec rm -rf {} + 2>/dev/null || true

# postinst makes some folders world writable; the extracting user owns everything instead
chmod -R u+rwX,go-w "$STAGE"

# 3. Add the portable launcher, desktop integration script and README
install -m 755 "$SCRIPT_DIR/o3de" "$STAGE/o3de"
install -m 755 "$SCRIPT_DIR/install-desktop-entry.sh" "$STAGE/install-desktop-entry.sh"

if [[ "$COMPRESSION" == xz ]]; then
    ARCHIVE_EXT=tar.xz
    COMPRESSOR=(xz -T0 -6)
else
    ARCHIVE_EXT=tar.gz
    COMPRESSOR=(gzip -6)
    if command -v pigz &> /dev/null; then
        COMPRESSOR=(pigz -6)
    fi
fi
ARCHIVE=$OUTPUT_DIR/$PACKAGE_NAME.$ARCHIVE_EXT

# Report the oldest glibc/libstdc++ the binaries can run on
MIN_GLIBC=unknown
MIN_GLIBCXX=unknown
if command -v objdump &> /dev/null; then
    echo "[portable] Scanning binaries for required glibc/libstdc++ versions"
    SYMBOL_VERSIONS=$(find "$STAGE/bin/Linux" -type f \( -name '*.so' -o -name '*.so.*' -o -perm -u+x \) \
        -exec objdump -T {} + 2>/dev/null | grep -oE 'GLIBC(XX)?_[0-9]+(\.[0-9]+)+' | sort -u || true)
    MIN_GLIBC=$(grep '^GLIBC_' <<< "$SYMBOL_VERSIONS" | sed 's/^GLIBC_//' | sort -V | tail -1 || true)
    MIN_GLIBCXX=$(grep '^GLIBCXX_' <<< "$SYMBOL_VERSIONS" | sed 's/^GLIBCXX_//' | sort -V | tail -1 || true)
    MIN_GLIBC=${MIN_GLIBC:-unknown}
    MIN_GLIBCXX=${MIN_GLIBCXX:-unknown}
else
    echo "[portable] Warning: objdump not found (binutils), README will not list the minimum glibc"
fi
sed -e "s|@VERSION@|$VERSION|g" \
    -e "s|@TOP_DIR@|$TOP_DIR|g" \
    -e "s|@PACKAGE_NAME@|$PACKAGE_NAME|g" \
    -e "s|@ARCHIVE_EXT@|$ARCHIVE_EXT|g" \
    -e "s|@MIN_GLIBC@|$MIN_GLIBC|g" \
    -e "s|@MIN_GLIBCXX@|$MIN_GLIBCXX|g" \
    "$SCRIPT_DIR/README-linux.txt" > "$STAGE/README-linux.txt"
chmod 644 "$STAGE/README-linux.txt"

# 4. Archive, checksum and optionally sign
echo "[portable] Creating $ARCHIVE"
tar -C "$STAGING_ROOT" --sort=name --owner=0 --group=0 --numeric-owner -cf - "$TOP_DIR" | "${COMPRESSOR[@]}" > "$ARCHIVE"

(cd "$OUTPUT_DIR" && sha256sum "$(basename "$ARCHIVE")" > "$(basename "$ARCHIVE").sha256")

if [[ -n "$SIGN_KEY" ]]; then
    echo "[portable] Signing with key $SIGN_KEY"
    gpg --batch --yes --local-user "$SIGN_KEY" --armor --detach-sign --output "$ARCHIVE.asc" "$ARCHIVE"
fi

if [[ "$KEEP_STAGING" == true ]]; then
    echo "[portable] Staging directory kept at $STAGE"
fi

echo "[portable] Minimum glibc: $MIN_GLIBC, minimum libstdc++ (GLIBCXX): $MIN_GLIBCXX"
echo "[portable] Done:"
ls -lh "$ARCHIVE"*
