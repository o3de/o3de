#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

add_subdirectory(NumericalMethods)

ly_get_list_relative_pal_filename(pal_source_dir ${CMAKE_CURRENT_LIST_DIR}/Source/Platform/${PAL_PLATFORM_NAME})

include(${pal_source_dir}/PAL_${PAL_PLATFORM_NAME_LOWERCASE}.cmake) # for PAL_TRAIT_PHYSX_SUPPORTED
set(LY_PHYSX_ENABLE_RUNNING_BENCHMARKS OFF CACHE BOOL "Adds a target to allow running of the physx benchmarks.")

if(PAL_TRAIT_PHYSX_SUPPORTED)
    set(physx_dependency 3rdParty::PhysX)
    set(physx_files physx_files.cmake)
    set(physx_shared_files physx_shared_files.cmake)
    set(physx_mock_files physx_mocks_files.cmake)
    set(physx_editor_files physx_editor_files.cmake)
else()
    set(physx_files physx_unsupported_files.cmake)
    set(physx_shared_files physx_unsupported_shared_files.cmake)
    set(physx_editor_files physx_unsupported_files.cmake)
endif()

ly_add_target(
    NAME PhysX.Static STATIC
    NAMESPACE Gem
    FILES_CMAKE
        ${physx_files}
    COMPILE_DEFINITIONS
        PUBLIC
            PHYSX_ENABLE_MULTI_THREADING
            $<$<NOT:$<CONFIG:release>>:AZ_PHYSICS_DEBUG_ENABLED>
    INCLUDE_DIRECTORIES
        PUBLIC
            .
            Source
            ${pal_source_dir}
            Include
    BUILD_DEPENDENCIES
        PUBLIC
            ${physx_dependency}
            AZ::AzCore
            AZ::AzFramework
            Legacy::CryCommon
        PRIVATE
            Gem::LmbrCentral
)

ly_add_target(
    NAME PhysX ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
    NAMESPACE Gem
    OUTPUT_NAME PhysX.Gem
    FILES_CMAKE
        ${physx_shared_files}
    COMPILE_DEFINITIONS
        PUBLIC
            PHYSX_ENABLE_MULTI_THREADING
    INCLUDE_DIRECTORIES
        PRIVATE
            ${pal_source_dir}
        PUBLIC
            Include
    BUILD_DEPENDENCIES
        PRIVATE
            Gem::PhysX.Static
            Gem::LmbrCentral
    RUNTIME_DEPENDENCIES
        Gem::LmbrCentral
)

# use the PhysX module in clients and servers:
ly_create_alias(NAME PhysX.Clients NAMESPACE Gem TARGETS Gem::PhysX)
ly_create_alias(NAME PhysX.Servers NAMESPACE Gem TARGETS Gem::PhysX)

if(PAL_TRAIT_BUILD_HOST_TOOLS)

    ly_add_target(
        NAME PhysX.Editor.Static STATIC
        NAMESPACE Gem
        AUTOMOC
        AUTOUIC
        FILES_CMAKE
            ${physx_editor_files}
        COMPILE_DEFINITIONS
            PUBLIC
                PHYSX_EDITOR
                ENABLE_NON_COMPILED_CGF
        INCLUDE_DIRECTORIES
            PRIVATE
                .
                Source
                ${pal_source_dir}
            PUBLIC
                Include
        BUILD_DEPENDENCIES
            PUBLIC
                ${physx_dependency}
                3rdParty::poly2tri
                3rdParty::v-hacd
                Legacy::Editor.Headers
                AZ::AzCore
                AZ::AzFramework
                AZ::AzToolsFramework
                AZ::GFxFramework
                AZ::AssetBuilderSDK
                AZ::SceneCore
                AZ::SceneData
                Legacy::CryCommon
                Gem::LmbrCentral.Editor
                Gem::PhysX.NumericalMethods
                Gem::PhysX.Static
                Gem::AtomLyIntegration_CommonFeatures.Static
        RUNTIME_DEPENDENCIES
            AZ::SceneCore
    )

    ly_add_target(
        NAME PhysX.Editor GEM_MODULE
        NAMESPACE Gem
        OUTPUT_NAME PhysX.Editor.Gem
        AUTOMOC
        FILES_CMAKE
            physx_editor_shared_files.cmake
        COMPILE_DEFINITIONS
            PUBLIC
                PHYSX_ENABLE_MULTI_THREADING
        INCLUDE_DIRECTORIES
            PRIVATE
                .
                Source
                ${pal_source_dir}
            PUBLIC
                Include
        BUILD_DEPENDENCIES
            PRIVATE
                Gem::PhysX.Editor.Static
        RUNTIME_DEPENDENCIES
            Gem::LmbrCentral.Editor
    )

    # use the PhysX.Editor module in dev tools:
    ly_create_alias(NAME PhysX.Builders NAMESPACE Gem TARGETS Gem::PhysX.Editor)
    ly_create_alias(NAME PhysX.Tools    NAMESPACE Gem TARGETS Gem::PhysX.Editor)

endif()

################################################################################
# Tests
################################################################################
if(PAL_TRAIT_BUILD_TESTS_SUPPORTED)
    ly_add_target(
        NAME PhysX.Mocks HEADERONLY
        NAMESPACE Gem
        OUTPUT_NAME PhysX.Mocks.Gem
        FILES_CMAKE
            physx_mocks_files.cmake
        INCLUDE_DIRECTORIES
            INTERFACE
                Mocks
    )

    ly_add_target(
        NAME PhysX.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
        NAMESPACE Gem
        OUTPUT_NAME PhysX.Tests.Gem
        FILES_CMAKE
            physx_tests_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Tests
        BUILD_DEPENDENCIES
            PRIVATE
                AZ::AzTestShared
                AZ::AzTest
                Gem::PhysX.Static
                Gem::LmbrCentral
        RUNTIME_DEPENDENCIES
            Gem::LmbrCentral
    )

    if(PAL_TRAIT_JOINTS_TYPED_TEST_CASE)
        ly_add_source_properties(
            SOURCES Tests/PhysXJointsTest.cpp
            PROPERTY COMPILE_DEFINITIONS 
            VALUES ENABLE_JOINTS_TYPED_TEST_CASE
        )
    endif()

    ly_add_googletest(
        NAME Gem::PhysX.Tests
    )

    # Only add the physx benchmarks if this flag is set. The benchmark code is still built, as it is part of the PhysX.Tests project.
    # Currently jenkins has a 1500sec(25min) timeout, our benchmarks can sometimes take over 1500sec and cause a build failure for timeout.
    # Jenkins currently doesn't upload the results of the benchmarks, so this is ok.
    if(LY_PHYSX_ENABLE_RUNNING_BENCHMARKS)
        ly_add_googlebenchmark(
            NAME Gem::PhysX.Benchmarks
            TARGET Gem::PhysX.Tests
        )
    endif()

    list(APPEND testTargets PhysX.Tests)
    
    if (PAL_TRAIT_BUILD_HOST_TOOLS)
        ly_add_target(
            NAME PhysX.Editor.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
            NAMESPACE Gem
            OUTPUT_NAME PhysX.Editor.Tests.Gem
            FILES_CMAKE
                physx_editor_tests_files.cmake
            INCLUDE_DIRECTORIES
                PRIVATE
                    Tests
                    Source
                    .
            BUILD_DEPENDENCIES
                PRIVATE
                    3rdParty::Qt::Test
                    AZ::AzTestShared
                    AZ::AzTest
                    AZ::AzToolsFrameworkTestCommon
                    Gem::PhysX.Static
                    Gem::PhysX.Mocks
                    Gem::PhysX.Editor.Static
            RUNTIME_DEPENDENCIES
                Gem::LmbrCentral.Editor
        )
        ly_add_googletest(
            NAME Gem::PhysX.Editor.Tests
        )
    endif()
    
    ly_add_target_files(
        TARGETS
            ${testTargets}
        FILES
            ${CMAKE_CURRENT_SOURCE_DIR}/Tests/RagdollConfiguration.xml
        OUTPUT_SUBDIRECTORY
            Test.Assets/Gems/PhysX/Code/Tests
            )
endif()