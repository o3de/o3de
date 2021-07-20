#
# Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
# 
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

set(LY_ENABLE_RAD_TELEMETRY OFF CACHE BOOL "Enables RAD Telemetry in Debug/Profile mode.")
set(LY_RAD_TELEMETRY_INSTALL_ROOT "${LY_3RDPARTY_PATH}/RadTelemetry" CACHE PATH "Install path to RAD Telemetry.")

ly_get_list_relative_pal_filename(pal_source_dir ${CMAKE_CURRENT_LIST_DIR}/Source/Platform/${PAL_PLATFORM_NAME})

ly_add_target(
    NAME RADTelemetry.Static STATIC
    NAMESPACE Gem
    FILES_CMAKE
        radtelemetry_files.cmake
        ${pal_source_dir}/platform_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Source
            ${pal_source_dir}
    BUILD_DEPENDENCIES
        PUBLIC
            AZ::AzCore
            Legacy::CryCommon
)

ly_add_target(
    NAME RADTelemetry ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
    NAMESPACE Gem
    FILES_CMAKE
        radtelemetry_shared_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Source
    BUILD_DEPENDENCIES
        PRIVATE
            Gem::RADTelemetry.Static
)

# the RADTelemetry module above can be used in all kinds of applications, but we don't enable it in asset builders
ly_create_alias(NAME RADTelemetry.Clients NAMESPACE Gem TARGETS Gem::RADTelemetry)
ly_create_alias(NAME RADTelemetry.Tools   NAMESPACE Gem TARGETS Gem::RADTelemetry)
ly_create_alias(NAME RADTelemetry.Servers NAMESPACE Gem TARGETS Gem::RADTelemetry)
