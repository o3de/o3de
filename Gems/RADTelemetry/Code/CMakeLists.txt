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
    OUTPUT_NAME Gem.RADTelemetry.bdaf32823406492686e35200afc555b3.v0.1.0
    FILES_CMAKE
        radtelemetry_shared_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Source
    BUILD_DEPENDENCIES
        PRIVATE
            Gem::RADTelemetry.Static
)
