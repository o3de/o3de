#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

o3de_gem_setup("PhysX")

ly_add_external_target_path(${CMAKE_CURRENT_SOURCE_DIR}/3rdParty)

o3de_pal_dir(pal_source_dir ${CMAKE_CURRENT_LIST_DIR}/Source/Platform/${PAL_PLATFORM_NAME} "${gem_restricted_path}" "${gem_path}" "${gem_parent_relative_path}")

include(${pal_source_dir}/PAL_${PAL_PLATFORM_NAME_LOWERCASE}.cmake) # for PAL_TRAIT_PHYSX_SUPPORTED

if(PAL_TRAIT_PHYSX_SUPPORTED)
    set(physx_dependency 3rdParty::PhysX4)
    set(physx_files ../Code/physx_files.cmake)
    set(physx_module_files physx_module_files.cmake)
    set(physx_shared_files ../Code/physx_shared_files.cmake)
    set(physx_editor_files ../Code/physx_editor_files.cmake)
else()
    set(physx_files ../Code/physx_unsupported_files.cmake)
    set(physx_module_files physx_unsupported_module_files.cmake)
    set(physx_shared_files ../Code/physx_unsupported_shared_files.cmake)
    set(physx_editor_files ../Code/physx_unsupported_files.cmake)
endif()

ly_add_target(
    NAME ${gem_name}.Static STATIC
    NAMESPACE Gem
    FILES_CMAKE
        ${physx_files}
    COMPILE_DEFINITIONS
        PUBLIC
            PHYSX_ENABLE_MULTI_THREADING
            PHYSX_SETREG_GEM_NAME="PhysX"
            $<$<NOT:$<CONFIG:release>>:AZ_PHYSICS_DEBUG_ENABLED>
    INCLUDE_DIRECTORIES
        PUBLIC
            ../Code/.
            ../Code/Source
            ${pal_source_dir}
            ../Code/Include
    BUILD_DEPENDENCIES
        PUBLIC
            ${physx_dependency}
            AZ::AzCore
            AZ::AzFramework
            Legacy::CryCommon
        PRIVATE
            Gem::LmbrCentral.API
)

ly_add_target(
    NAME ${gem_name} ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
    NAMESPACE Gem
    OUTPUT_NAME ${gem_name}.Gem
    FILES_CMAKE
        ${physx_module_files}
        ${physx_shared_files}
    COMPILE_DEFINITIONS
        PUBLIC
            PHYSX_ENABLE_MULTI_THREADING
    INCLUDE_DIRECTORIES
        PRIVATE
            ${pal_source_dir}
        PUBLIC
            ../Code/Include
    BUILD_DEPENDENCIES
        PRIVATE
            Gem::${gem_name}.Static
            Gem::LmbrCentral.API
    RUNTIME_DEPENDENCIES
        Gem::LmbrCentral
)

# Inject the gem name into the Module source file
if(PAL_TRAIT_PHYSX_SUPPORTED)
    ly_add_source_properties(
        SOURCES
            Source/Module.cpp
        PROPERTY COMPILE_DEFINITIONS
            VALUES
                O3DE_GEM_NAME=${gem_name}
                O3DE_GEM_VERSION=${gem_version})
else()
    ly_add_source_properties(
        SOURCES
            Source/ModuleUnsupported.cpp
        PROPERTY COMPILE_DEFINITIONS
            VALUES
                O3DE_GEM_NAME=${gem_name}
                O3DE_GEM_VERSION=${gem_version})
endif()

# use the ${gem_name} module in clients and servers:
ly_create_alias(NAME ${gem_name}.Clients NAMESPACE Gem TARGETS Gem::${gem_name})
ly_create_alias(NAME ${gem_name}.Servers NAMESPACE Gem TARGETS Gem::${gem_name})
ly_create_alias(NAME ${gem_name}.Unified NAMESPACE Gem TARGETS Gem::${gem_name})

if(PAL_TRAIT_BUILD_HOST_TOOLS)

    ly_add_target(
        NAME ${gem_name}.Editor.Static STATIC
        NAMESPACE Gem
        AUTOMOC
        AUTOUIC
        FILES_CMAKE
            ${physx_editor_files}
        PLATFORM_INCLUDE_FILES
            ${CMAKE_CURRENT_LIST_DIR}/Source/Platform/Common/${PAL_TRAIT_COMPILER_ID}/physx_editor_static_${PAL_TRAIT_COMPILER_ID_LOWERCASE}.cmake
        COMPILE_DEFINITIONS
            PUBLIC
                PHYSX_EDITOR
                ENABLE_NON_COMPILED_CGF
        INCLUDE_DIRECTORIES
            PRIVATE
                ../Code/.
                ../Code/Source
                ${pal_source_dir}
            PUBLIC
                ../Code/Include
        BUILD_DEPENDENCIES
            PUBLIC
                ${physx_dependency}
                3rdParty::poly2tri
                3rdParty::v-hacd
                Legacy::Editor.Headers
                AZ::AzCore
                AZ::AzFramework
                AZ::AzToolsFramework
                AZ::AssetBuilderSDK
                AZ::SceneCore
                AZ::SceneData
                Legacy::CryCommon
                Gem::LmbrCentral.Editor
                Gem::PhysX.NumericalMethods
                Gem::${gem_name}.Static
                Gem::CommonFeaturesAtom.Static
        RUNTIME_DEPENDENCIES
            AZ::SceneCore
    )

    ly_add_target(
        NAME ${gem_name}.Editor GEM_MODULE
        NAMESPACE Gem
        OUTPUT_NAME ${gem_name}.Editor.Gem
        AUTOMOC
        FILES_CMAKE
            ${physx_module_files}
        COMPILE_DEFINITIONS
            PUBLIC
                PHYSX_ENABLE_MULTI_THREADING
        INCLUDE_DIRECTORIES
            PRIVATE
                ../Code/.
                ../Code/Source
                ${pal_source_dir}
            PUBLIC
                ../Code/Include
        BUILD_DEPENDENCIES
            PRIVATE
                Gem::${gem_name}.Editor.Static
    )

    # use the ${gem_name}.Editor module in dev tools:
    ly_create_alias(NAME ${gem_name}.Builders NAMESPACE Gem TARGETS Gem::${gem_name}.Editor)
    ly_create_alias(NAME ${gem_name}.Tools    NAMESPACE Gem TARGETS Gem::${gem_name}.Editor)

endif()

################################################################################
# Tests
################################################################################
if(PAL_TRAIT_BUILD_TESTS_SUPPORTED)
    ly_add_target(
        NAME ${gem_name}.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
        NAMESPACE Gem
        OUTPUT_NAME ${gem_name}.Tests.Gem
        FILES_CMAKE
            ../Code/physx_tests_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                ../Code/Tests
        BUILD_DEPENDENCIES
            PRIVATE
                AZ::AzTestShared
                AZ::AzTest
                Gem::${gem_name}.Static
                Gem::LmbrCentral
        RUNTIME_DEPENDENCIES
            Gem::LmbrCentral
    )

    ly_add_googletest(
        NAME Gem::${gem_name}.Tests
        LABELS REQUIRES_tiaf
    )

    # Only add the physx benchmarks if this flag is set. The benchmark code is still built, as it is part of the ${gem_name}.Tests project.
    # Currently jenkins has a 1500sec(25min) timeout, our benchmarks can sometimes take over 1500sec and cause a build failure for timeout.
    # Jenkins currently doesn't upload the results of the benchmarks, so this is ok.
    if(LY_PHYSX_ENABLE_RUNNING_BENCHMARKS)
        ly_add_googlebenchmark(
            NAME Gem::${gem_name}.Benchmarks
            TARGET Gem::${gem_name}.Tests
        )
    endif()

    list(APPEND testTargets ${gem_name}.Tests)
    
    if (PAL_TRAIT_BUILD_HOST_TOOLS)
        ly_add_target(
            NAME ${gem_name}.Editor.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
            NAMESPACE Gem
            OUTPUT_NAME ${gem_name}.Editor.Tests.Gem
            FILES_CMAKE
                ${physx_module_files}
                ../Code/physx_editor_tests_files.cmake
            INCLUDE_DIRECTORIES
                PRIVATE
                    ../Code/Tests
                    ../Code/Source
                    ../Code/.
            BUILD_DEPENDENCIES
                PRIVATE
                    3rdParty::Qt::Test
                    AZ::AzTestShared
                    AZ::AzTest
                    AZ::AzToolsFrameworkTestCommon
                    AZ::AzManipulatorTestFramework.Static
                    Gem::${gem_name}.Static
                    Gem::PhysX.Mocks
                    Gem::${gem_name}.Editor.Static
            RUNTIME_DEPENDENCIES
                Gem::LmbrCentral.Editor
        )
        ly_add_googletest(
            NAME Gem::${gem_name}.Editor.Tests
            LABELS REQUIRES_tiaf
        )
    endif()
endif()
