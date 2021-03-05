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

ly_get_list_relative_pal_filename(pal_dir ${CMAKE_CURRENT_LIST_DIR}/Platform/${PAL_PLATFORM_NAME})
include(${pal_dir}/PAL_${PAL_PLATFORM_NAME_LOWERCASE}.cmake)

if(NOT PAL_TRAIT_BUILD_RESOURCECOMPILERLEGACY_SUPPORTED OR NOT PAL_TRAIT_BUILD_HOST_TOOLS)
    return()
endif()

ly_add_target(
    NAME ResourceCompilerLegacy.Static STATIC
    NAMESPACE Legacy
    FILES_CMAKE
        resourcecompilerlegacy_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            .
    BUILD_DEPENDENCIES
        PRIVATE
            AZ::SceneCore
        PUBLIC
            Legacy::CryCommon
            Legacy::CryCommonTools
            Legacy::ResourceCompiler.Static
            AZ::AssetBuilderSDK
)

ly_add_target(
    NAME ResourceCompilerLegacy MODULE
    NAMESPACE Legacy
    OUTPUT_SUBDIRECTORY rc_plugins
    FILES_CMAKE
        resourcecompilerlegacy_shared_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            .
    BUILD_DEPENDENCIES
        PRIVATE
            Legacy::ResourceCompilerLegacy.Static
)

ly_add_dependencies(RC Legacy::ResourceCompilerLegacy)

if(PAL_TRAIT_BUILD_TESTS_SUPPORTED)
    ly_add_target(
        NAME ResourceCompilerLegacy.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
        NAMESPACE Legacy
        FILES_CMAKE
            resourcecompilerlegacy_test_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Source
                Tests
                .
        BUILD_DEPENDENCIES
            PRIVATE
                AZ::AzTest
                Legacy::ResourceCompilerLegacy.Static
    )
    ly_add_googletest(
        NAME Legacy::ResourceCompilerLegacy.Tests
    )
endif()
