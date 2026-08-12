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

ios_simulator_sdk_version=""
if ! ios_simulator_sdk_version="$(xcrun --sdk iphonesimulator --show-sdk-version)"; then
    echo "Unable to determine the selected Xcode's iOS Simulator SDK version"
fi

if simulator_runtimes="$(xcrun simctl runtime list -j)"; then
    keep_runtime_identifier="$(
        jq -r --arg version "${ios_simulator_sdk_version}" '
            [.[] | select(
                .platformIdentifier == "com.apple.platform.iphonesimulator"
                and .version == $version
                and .state == "Ready"
            )]
            | first
            | .identifier // empty
        ' <<< "${simulator_runtimes}"
    )"

    if [[ -n "${keep_runtime_identifier}" ]]; then
        echo "Keeping iOS ${ios_simulator_sdk_version} simulator runtime"
    else
        echo "No simulator runtime matches the selected Xcode. Removing all simulator runtimes"
    fi

    while IFS=$'\t' read -r runtime_identifier runtime_platform runtime_version; do
        [[ -n "${runtime_identifier}" ]] || continue
        echo "Removing simulator runtime: ${runtime_platform} ${runtime_version}"
        if ! xcrun simctl runtime delete "${runtime_identifier}"; then
            echo "Unable to remove simulator runtime: ${runtime_identifier}"
        fi
    done < <(
        jq -r --arg keep "${keep_runtime_identifier}" '
            .[]
            | select(.deletable == true)
            | select((.platformIdentifier // "") | endswith("simulator"))
            | select(.identifier != $keep)
            | [.identifier, .platformIdentifier, .version]
            | @tsv
        ' <<< "${simulator_runtimes}"
    )

    xcrun simctl delete unavailable || true
else
    echo "Unable to list simulator runtimes. Skipping simulator cleanup"
fi

echo "Disk space after cleanup:"
df -h /
