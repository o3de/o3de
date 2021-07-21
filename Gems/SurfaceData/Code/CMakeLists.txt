#
# Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
# 
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
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
            Gem::LmbrCentral
        PUBLIC
            Gem::Atom_RPI.Public
            Gem::Atom_Feature_Common.Static
)

ly_add_target(
    NAME SurfaceData ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
    NAMESPACE Gem
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
            Gem::LmbrCentral
    RUNTIME_DEPENDENCIES
        Gem::LmbrCentral
)

# the above module is for use in all client/server types 
ly_create_alias(NAME SurfaceData.Servers  NAMESPACE Gem TARGETS Gem::SurfaceData)
ly_create_alias(NAME SurfaceData.Clients  NAMESPACE Gem TARGETS Gem::SurfaceData)

if (PAL_TRAIT_BUILD_HOST_TOOLS)

    ly_add_target(
        NAME SurfaceData.Editor GEM_MODULE

        NAMESPACE Gem
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
                Gem::LmbrCentral.Editor
        RUNTIME_DEPENDENCIES
            Gem::LmbrCentral.Editor
    )
    # the above module is for use in dev tool situations
    ly_create_alias(NAME SurfaceData.Builders NAMESPACE Gem TARGETS Gem::SurfaceData.Editor)
    ly_create_alias(NAME SurfaceData.Tools    NAMESPACE Gem TARGETS Gem::SurfaceData.Editor)

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
                Gem::LmbrCentral
    )
    ly_add_googletest(
        NAME Gem::SurfaceData.Tests
    )
endif()
