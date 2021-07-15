#
# Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
# 
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

# TODO: would like to be able to build from this path, however, the whole setup is done at the workspace's root
#       we also dont want to drop cmake output files everywhere.

ly_get_list_relative_pal_filename(pal_dir ${CMAKE_CURRENT_LIST_DIR}/Platform/${PAL_PLATFORM_NAME})
ly_get_list_relative_pal_filename(common_dir ${CMAKE_CURRENT_LIST_DIR}/Platform/Common)

if(LY_ENABLE_RAD_TELEMETRY)
    set(AZ_CORE_RADTELEMETRY_FILES ${common_dir}/azcore_profile_telemetry_files.cmake)
    set(AZ_CORE_RADTELEMETRY_PLATFORM_INCLUDES ${pal_dir}/profile_telemetry_platform_${PAL_PLATFORM_NAME_LOWERCASE}.cmake)
    set(AZ_CORE_RADTELEMETRY_INCLUDE_DIRECTORIES ${common_dir})
    set(AZ_CORE_RADTELEMETRY_BUILD_DEPENDENCIES 3rdParty::RadTelemetry)
endif()

ly_add_target(
    NAME AzCore STATIC
    NAMESPACE AZ
    FILES_CMAKE
        AzCore/azcore_files.cmake
        AzCore/std/azstd_files.cmake
        ${pal_dir}/platform_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake
        ${AZ_CORE_RADTELEMETRY_FILES}
    PLATFORM_INCLUDE_FILES
        ${pal_dir}/platform_${PAL_PLATFORM_NAME_LOWERCASE}.cmake
        ${AZ_CORE_RADTELEMETRY_PLATFORM_INCLUDES}
    INCLUDE_DIRECTORIES
        PUBLIC
            .
            ${pal_dir}
            ${common_dir}
            ${AZ_CORE_RADTELEMETRY_INCLUDE_DIRECTORIES}
    BUILD_DEPENDENCIES
        PUBLIC
            3rdParty::Lua
            3rdParty::RapidJSON
            3rdParty::RapidXML
            3rdParty::zlib
            3rdParty::zstd
            3rdParty::cityhash
            ${AZ_CORE_RADTELEMETRY_BUILD_DEPENDENCIES}
)
ly_add_source_properties(
    SOURCES
        AzCore/Script/ScriptSystemComponent.cpp
        AzCore/PlatformId/PlatformId.cpp
    PROPERTY COMPILE_DEFINITIONS
    VALUES ${LY_PAL_TOOLS_DEFINES}
)

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
                AZ::AzTest
    )

    ly_get_list_relative_pal_filename(pal_test_dir ${CMAKE_CURRENT_LIST_DIR}/Tests/Platform/${PAL_PLATFORM_NAME})
    ly_add_target(
        NAME AzCore.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
        NAMESPACE AZ
        FILES_CMAKE
            Tests/azcoretests_files.cmake
            ${pal_test_dir}/platform_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake
        PLATFORM_INCLUDE_FILES
            ${pal_test_dir}/platform_${PAL_PLATFORM_NAME_LOWERCASE}.cmake
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

endif()

