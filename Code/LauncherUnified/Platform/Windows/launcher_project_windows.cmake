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

set(SPLASH_FILE ${project_real_path}/Resources/Splash.bmp)
if(NOT EXISTS ${SPLASH_FILE})
    # Try in project Gem
    set(SPLASH_FILE ${project_real_path}/Gem/Resources/Splash.bmp)
endif()
if(NOT EXISTS ${SPLASH_FILE})
    # Try legacy splash
    set(SPLASH_FILE ${project_real_path}/Resources/LegacyLogoLauncher.bmp)
endif()
if(NOT EXISTS ${SPLASH_FILE})
    # Try in Gem
    set(SPLASH_FILE ${project_real_path}/Gem/Resources/LegacyLogoLauncher.bmp)
endif()

if(EXISTS ${ICON_FILE} OR EXISTS ${SPLASH_FILE})
    set(target_file ${CMAKE_CURRENT_BINARY_DIR}/${project_name}.GameLauncher.rc)
    configure_file(${CMAKE_CURRENT_LIST_DIR}/Launcher.rc.in
        ${target_file}
        @ONLY
    )
    set(LY_FILES ${target_file})
endif()
