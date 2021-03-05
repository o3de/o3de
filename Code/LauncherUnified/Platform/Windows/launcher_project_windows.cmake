#
# All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
# its licensors.
#
# For complete copyright and license terms please see the LICENSE at the root of this
# distribution (the "License"). All use of this software is governed by the License,
# or, if provided, by the license below or the license accompanying this file. Do not
# remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#

if (LY_MONOLITHIC_GAME) # only Atom is supported in monolithic
    set(LY_BUILD_DEPENDENCIES
        PUBLIC
            Legacy::CryRenderOther
    )
else()
    set(LY_BUILD_DEPENDENCIES
        PRIVATE
            Legacy::CryRenderD3D11
    )
endif()

# Find the resource from the game gem
get_target_property(${project}_GEM_DIR ${project} SOURCE_DIR) # Point to where the code is
get_filename_component(${project}_GEM_DIR ${${project}_GEM_DIR} DIRECTORY) # Parent directory

set(ICON_FILE ${${project}_GEM_DIR}/Resources/GameSDK.ico)
if(NOT EXISTS ${ICON_FILE})
    # Try the common LauncherUnified icon instead
    set(ICON_FILE Resources/GameSDK.ico)
endif()
if(EXISTS ${ICON_FILE})
    set(target_file ${CMAKE_CURRENT_BINARY_DIR}/${project}.GameLauncher.rc)
    configure_file(Platform/Windows/Launcher.rc.in
        ${target_file}
        @ONLY
    )
    set(LY_FILES ${target_file})
endif()
