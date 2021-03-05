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

if(NOT PAL_TRAIT_BUILD_CRASH_HANDLER_SUPPORTED)
    return()
endif()

ly_add_target(
    NAME CrashReporting ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
    NAMESPACE Gem
    OUTPUT_NAME Gem.CrashReporting.089562a2cbbd41749b359f85fa04f1c9.v0.1.0
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
            3rdParty::Crashpad
            3rdParty::Crashpad::Handler
            AZ::CrashHandler
            AZ::CrashSupport
            Legacy::CryCommon
)

ly_add_target(
    NAME CrashReporting.Uploader APPLICATION
    NAMESPACE AZ
    FILES_CMAKE
        Platform/${PAL_PLATFORM_NAME}/game_crash_uploader_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Include
    BUILD_DEPENDENCIES
        PRIVATE
            3rdParty::Crashpad
            3rdParty::Crashpad::Handler
            AZ::AzCore
            AZ::CrashUploaderSupport
            AZ::CrashHandler
)
