#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

# Keeps the Asset Processor from scanning a project's CMake build tree.
#
# When the CMake binary directory lives inside a project (e.g. an in-source
# CLion "cmake-build-*" folder), the engine's default "ScanFolder Project/Assets"
# scan folder (@PROJECTROOT@) descends into it and tries to process build
# artifacts as source assets.

function(o3de_exclude_build_dir_from_asset_processor project_root)
    file(RELATIVE_PATH build_rel "${project_root}" "${CMAKE_BINARY_DIR}")

    # Out-of-source build: Asset Processor never scans it, so nothing to do.
    if(IS_ABSOLUTE "${build_rel}" OR build_rel MATCHES "^\\.\\.")
        return()
    endif()

    # Filesystem-safe, unique id so multiple in-tree build dirs each keep their
    # own overlay (reconfiguring the same dir just overwrites its own file).
    string(REGEX REPLACE "[^A-Za-z0-9]+" "_" build_id "${build_rel}")

    set(overlay "${project_root}/user/Registry/asset_processor_exclude_${build_id}.setreg")
    file(WRITE "${overlay}" "\
{
    \"Amazon\": {
        \"AssetProcessor\": {
            \"Settings\": {
                \"Exclude ProjectBuildDir ${build_id}\": {
                    \"glob\": \"${build_rel}/*\"
                }
            }
        }
    }
}
")
    message(STATUS "Asset Processor: excluding in-tree build dir '${build_rel}/' (overlay: ${overlay})")
endfunction()
