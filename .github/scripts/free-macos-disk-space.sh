#!/usr/bin/env bash
#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#

set -euo pipefail

if [[ "${GITHUB_ACTIONS:-}" != "true" || "${RUNNER_ENVIRONMENT:-}" != "github-hosted" ]]; then
    echo "Skipping disk cleanup outside a GitHub-hosted runner"
    exit 0
fi

if [[ "${RUNNER_OS:-}" != "macOS" ]]; then
    echo "Skipping disk cleanup on ${RUNNER_OS}"
    exit 0
fi

echo "Disk space before cleanup:"
df -h /

selected_xcode="$(xcode-select -p)"
selected_xcode="${selected_xcode%/Contents/Developer}"
selected_xcode="$(realpath "${selected_xcode}")"
echo "Keeping selected Xcode installation: ${selected_xcode}"

for xcode in /Applications/Xcode*.app; do
    [[ -e "${xcode}" ]] || continue

    resolved_xcode="$(realpath "${xcode}")"
    if [[ "${resolved_xcode}" == "${selected_xcode}" ]]; then
        continue
    fi

    if [[ "${resolved_xcode}" != /Applications/Xcode*.app ]]; then
        echo "Skipping unexpected Xcode path: ${xcode} -> ${resolved_xcode}"
        continue
    fi

    echo "Removing unused Xcode installation: ${xcode}"
    sudo rm -rf "${xcode}"
done

echo "Disk space after cleanup:"
df -h /
