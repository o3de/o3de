#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

# TODO: would like to be able to build from this path, however, the whole setup is done at the workspace's root
#       we also dont want to drop cmake output files everywhere.

o3de_pal_dir(pal_dir ${CMAKE_CURRENT_LIST_DIR}/Platform/${PAL_PLATFORM_NAME} ${O3DE_ENGINE_RESTRICTED_PATH} ${LY_ROOT_FOLDER})
set(common_dir ${CMAKE_CURRENT_LIST_DIR}/Platform/Common)

if(PAL_TRAIT_PROF_PIX_SUPPORTED)
    set(LY_PIX_ENABLED OFF CACHE BOOL "Enables PIX profiler integration.")
    if(LY_PIX_ENABLED)
        message(STATUS "PIX found, enabling as profiler")
        set(AZ_CORE_PIX_BUILD_DEPENDENCIES 3rdParty::pix)
    endif()
endif()

# By default, the Settings Registry will apply user and project registry files as overrides in development (non-release) builds,
# and will ignore all overrides in release builds.
# This behavior can be overridden by passing -DALLOW_SETTINGS_REGISTRY_DEVELOPMENT_OVERRIDES=0, 1, 2 or 3 when generating the build files.
# The override can be removed by passing -UALLOW_SETTINGS_REGISTRY_DEVELOPMENT_OVERRIDES when generating the build files.
set(ALLOW_SETTINGS_REGISTRY_DEVELOPMENT_OVERRIDES "" CACHE STRING 
"Forces the Settings Registry development overrides to be used or ignored.
  If unset, development overrides are used in development builds and ignored in release builds.
  0 disables the development overrides in all builds.
  1 = registry overrides are allowed from the command line.
  2 = registry overrides are allowed from the command line, engine, gem, and project files.
  3 = registry overrides are allowed from the command line, engine, gem, project, and user files.")

# If there's a value in ALLOW_SETTINGS_REGISTRY_DEVELOPMENT_OVERRIDES, the compiler flag will get set to it.
# Otherwise, the variable will be empty and no compiler flag will be set, leaving it to the code to decide
# whether or not to allow the Settings Registry development overrides.
set(ALLOW_SETTINGS_REGISTRY_DEVELOPMENT_OVERRIDES_FLAG $<$<NOT:$<STREQUAL:"${ALLOW_SETTINGS_REGISTRY_DEVELOPMENT_OVERRIDES}","">>:ALLOW_SETTINGS_REGISTRY_DEVELOPMENT_OVERRIDES=${ALLOW_SETTINGS_REGISTRY_DEVELOPMENT_OVERRIDES}>)



ly_add_target(
    NAME O3DEKernel ${PAL_TRAIT_MONOLITHIC_DRIVEN_LIBRARY_TYPE}
    NAMESPACE AZ
    FILES_CMAKE
        AzCore/o3dekernel_files.cmake
    PLATFORM_INCLUDE_FILES
        ${pal_dir}/o3dekernel_${PAL_PLATFORM_NAME_LOWERCASE}.cmake
    INCLUDE_DIRECTORIES
        PUBLIC
            .
            ${pal_dir}
            ${common_dir}
)

ly_add_target(
    NAME AzCore STATIC
    NAMESPACE AZ
    FILES_CMAKE
        AzCore/azcore_files.cmake
        AzCore/std/azstd_files.cmake
        ${pal_dir}/platform_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake
    PLATFORM_INCLUDE_FILES
        ${pal_dir}/platform_${PAL_PLATFORM_NAME_LOWERCASE}.cmake
    INCLUDE_DIRECTORIES
        PUBLIC
            .
            ${pal_dir}
            ${common_dir}
    BUILD_DEPENDENCIES
        PUBLIC
            3rdParty::Lua
            3rdParty::RapidJSON
            3rdParty::RapidXML
            3rdParty::ZLIB
            3rdParty::zstd
            3rdParty::cityhash
            ${AZ_CORE_PIX_BUILD_DEPENDENCIES}
            AZ::O3DEKernel
)
ly_add_source_properties(
    SOURCES
        AzCore/Script/ScriptSystemComponent.cpp
        AzCore/PlatformId/PlatformId.cpp
    PROPERTY COMPILE_DEFINITIONS
    VALUES ${LY_PAL_TOOLS_DEFINES}
)

ly_add_source_properties(
    SOURCES
        AzCore/Component/ComponentApplication.cpp
    PROPERTY COMPILE_DEFINITIONS
    VALUES ${ALLOW_SETTINGS_REGISTRY_DEVELOPMENT_OVERRIDES_FLAG}
)

# Add the O3DE_STACK_CAPTURE_DEPTH define only to the cpp files for the following allocators
# This reduces re-compile time when the cache variable value changes.
set(O3DE_STACK_CAPTURE_DEPTH_DEFINE $<$<NOT:$<STREQUAL:"${O3DE_STACK_CAPTURE_DEPTH}","">>:O3DE_STACK_CAPTURE_DEPTH=${O3DE_STACK_CAPTURE_DEPTH}>)
ly_add_source_properties(
    SOURCES
        AzCore/Memory/OSAllocator.cpp
        AzCore/Memory/PoolAllocator.cpp
        AzCore/Memory/SystemAllocator.cpp
    PROPERTY COMPILE_DEFINITIONS
    VALUES ${O3DE_STACK_CAPTURE_DEPTH_DEFINE}
)

if(LY_BUILD_WITH_ADDRESS_SANITIZER)
    # Default to use Malloc schema so ASan works well
    ly_add_source_properties(
        SOURCES AzCore/Memory/SystemAllocator.cpp
        PROPERTY COMPILE_DEFINITIONS
        VALUES AZCORE_SYSTEM_ALLOCATOR=AZCORE_SYSTEM_ALLOCATOR_MALLOC
    )
endif()

################################################################################
# Tests
################################################################################
if(PAL_TRAIT_BUILD_TESTS_SUPPORTED)

    ly_add_target(
        NAME AzCoreTestCommon STATIC
        NAMESPACE AZ
        FILES_CMAKE
            AzCore/azcoretestcommon_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                .
                ${pal_dir}
                ${common_dir}
        BUILD_DEPENDENCIES
            PRIVATE
                3rdParty::googletest::GMock
                3rdParty::GoogleBenchmark
    )

    ly_add_target(
        NAME AzTestShared STATIC
        NAMESPACE AZ
        FILES_CMAKE
            Tests/aztestshared_files.cmake
        INCLUDE_DIRECTORIES
            PUBLIC
                Tests
        BUILD_DEPENDENCIES
            PRIVATE
                AZ::AzTest
                AZ::AzCore
    )

    ly_add_target(
        NAME AzCoreTestDLL MODULE
        NAMESPACE AZ
        FILES_CMAKE
            Tests/azcoretestdll_files.cmake
        INCLUDE_DIRECTORIES
            PUBLIC
                Tests
        BUILD_DEPENDENCIES
            PRIVATE
                AZ::AzCore
    )

    o3de_pal_dir(pal_tests_dir ${CMAKE_CURRENT_LIST_DIR}/Tests/Platform/${PAL_PLATFORM_NAME} ${O3DE_ENGINE_RESTRICTED_PATH} ${LY_ROOT_FOLDER})
    ly_add_target(
        NAME AzCore.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
        NAMESPACE AZ
        FILES_CMAKE
            Tests/azcoretests_files.cmake
            ${pal_tests_dir}/platform_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake
        PLATFORM_INCLUDE_FILES
            ${pal_tests_dir}/platform_${PAL_PLATFORM_NAME_LOWERCASE}.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Tests
        COMPILE_DEFINITIONS
            PRIVATE
                AZ_NUMERICCAST_ENABLED
        BUILD_DEPENDENCIES
            PRIVATE
                AZ::AzCore
                AZ::AzCoreTestCommon
                AZ::AzTestShared
                AZ::AzTest
        RUNTIME_DEPENDENCIES
            AZ::AzCoreTestDLL
    )
    ly_add_googletest(
        NAME AZ::AzCore.Tests
        LABELS REQUIRES_tiaf
    )
    ly_add_googletest(
        NAME AZ::AzCore.Tests
        TEST_SUITE sandbox
    )
    ly_add_googlebenchmark(
        NAME AZ::AzCore.Benchmarks
        TARGET AZ::AzCore.Tests
    )
    ly_add_source_properties(
        SOURCES Tests/Debug.cpp
        PROPERTY COMPILE_DEFINITIONS
        VALUES AZCORETEST_DLL_NAME=\"$<TARGET_FILE_NAME:AzCore.Tests>\"
    )
    ly_add_target_files(
        TARGETS AzCore.Tests
        FILES ${CMAKE_CURRENT_SOURCE_DIR}/Tests/Memory/AllocatorBenchmarkRecordings.bin
        OUTPUT_SUBDIRECTORY Tests/AzCore/Memory
    )

endif()

