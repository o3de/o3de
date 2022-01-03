#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

ly_get_list_relative_pal_filename(pal_dir ${CMAKE_CURRENT_LIST_DIR}/Platform/${PAL_PLATFORM_NAME})

include(${pal_dir}/PAL_${PAL_PLATFORM_NAME_LOWERCASE}.cmake)

if(NOT PAL_TRAIT_BUILD_CRASH_HANDLER_SUPPORTED)
    return()
endif()

ly_add_target(
    NAME CrashReporting ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
    NAMESPACE Gem
    FILES_CMAKE
        crashreporting_static_files.cmake
        Platform/${PAL_PLATFORM_NAME}/crashreporting_static_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Source
        PUBLIC
            Include
    BUILD_DEPENDENCIES
        PRIVATE
            AZ::CrashHandler
            AZ::AzCore
)

# Load the "Gem::CrashReporting" module in Clients and Servers
ly_create_alias(NAME CrashReporting.Clients NAMESPACE Gem TARGETS Gem::CrashReporting)
ly_create_alias(NAME CrashReporting.Servers NAMESPACE Gem TARGETS Gem::CrashReporting)

if(NOT LY_MONOLITHIC_GAME)

    ly_add_target(
        NAME CrashReporting.Uploader APPLICATION
        NAMESPACE AZ
        FILES_CMAKE
            game_crash_uploader_files.cmake
            Platform/${PAL_PLATFORM_NAME}/game_crash_uploader_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Include
        BUILD_DEPENDENCIES
            PRIVATE
                AZ::CrashUploaderSupport
    )

endif()
