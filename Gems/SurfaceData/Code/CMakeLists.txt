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

ly_add_target(
    NAME SurfaceData.Static STATIC
    NAMESPACE Gem
    FILES_CMAKE
        surfacedata_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Source
        PUBLIC
            Include
    BUILD_DEPENDENCIES
        PRIVATE
            Legacy::CryCommon
        PUBLIC
            Gem::LmbrCentral
)

ly_add_target(
    NAME SurfaceData ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
    NAMESPACE Gem
    OUTPUT_NAME Gem.SurfaceData.5de82d29d6094bfe97c1a4d35fcd5fbe.v0.1.0
    FILES_CMAKE
        surfacedata_shared_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Source
        PUBLIC
            Include
    BUILD_DEPENDENCIES
        PRIVATE
            Legacy::CryCommon
            Gem::SurfaceData.Static
)

if (PAL_TRAIT_BUILD_HOST_TOOLS)

    ly_add_target(
        NAME SurfaceData.Editor MODULE
        NAMESPACE Gem
        OUTPUT_NAME Gem.SurfaceData.Editor.5de82d29d6094bfe97c1a4d35fcd5fbe.v0.1.0
        FILES_CMAKE
            surfacedata_editor_files.cmake
        COMPILE_DEFINITIONS
            PRIVATE
                SURFACEDATA_EDITOR
        INCLUDE_DIRECTORIES
            PRIVATE
                Source
            PUBLIC
                Include
        BUILD_DEPENDENCIES
            PRIVATE
                Legacy::CryCommon
                AZ::AzToolsFramework
                Gem::SurfaceData.Static
    )

endif()

################################################################################
# Tests
################################################################################
if(PAL_TRAIT_BUILD_TESTS_SUPPORTED)

    ly_add_target(
        NAME SurfaceData.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
        NAMESPACE Gem
        FILES_CMAKE
            surfacedata_tests_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                .
                Tests
                Source
        BUILD_DEPENDENCIES
            PRIVATE
                AZ::AzTest
                Legacy::CryCommon
                Gem::SurfaceData.Static
    )
    ly_add_googletest(
        NAME Gem::SurfaceData.Tests
    )
endif()
