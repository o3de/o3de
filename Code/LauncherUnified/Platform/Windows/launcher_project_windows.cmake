#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

set(ICON_FILE ${project_real_path}/Gem/Resources/GameSDK.ico)
if(NOT EXISTS ${ICON_FILE})
    # Try another project-relative path
    set(ICON_FILE ${project_real_path}/Resources/GameSDK.ico)
endif()

if(NOT EXISTS ${ICON_FILE})
    # Try the common LauncherUnified icon instead
    set(ICON_FILE Resources/GameSDK.ico)
endif()

if(EXISTS ${ICON_FILE})
    set(target_file ${CMAKE_CURRENT_BINARY_DIR}/${project_name}.GameLauncher.rc)
    configure_file(${CMAKE_CURRENT_LIST_DIR}/Launcher.rc.in
        ${target_file}
        @ONLY
    )
    set(LY_FILES ${target_file})
endif()
