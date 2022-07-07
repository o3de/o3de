#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

o3de_pal_dir(pal_source_dir ${CMAKE_CURRENT_LIST_DIR}/Source/Platform/${PAL_PLATFORM_NAME} "${gem_restricted_path}" "${gem_path}" "${gem_parent_relative_path}")
set(common_source_dir ${CMAKE_CURRENT_LIST_DIR}/Source/Platform/Common)

ly_add_target(
    NAME Metastream.Static STATIC
    NAMESPACE Gem
    FILES_CMAKE
        metastream_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Source
        PUBLIC
            Include
    BUILD_DEPENDENCIES
        PUBLIC
            AZ::AzCore
        PRIVATE
            Legacy::CryCommon
)

ly_add_target(
    NAME Metastream ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
    NAMESPACE Gem
    FILES_CMAKE
        metastream_shared_files.cmake
        ${pal_source_dir}/platform_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake
    PLATFORM_INCLUDE_FILES
        ${pal_source_dir}/metastream_${PAL_PLATFORM_NAME_LOWERCASE}.cmake
        ${common_source_dir}/${PAL_TRAIT_COMPILER_ID}/metastream_${PAL_TRAIT_COMPILER_ID_LOWERCASE}.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Source
            ${pal_source_dir}
        PUBLIC
            Include
    BUILD_DEPENDENCIES
        PRIVATE
            Gem::Metastream.Static
            Legacy::CryCommon
)

# The above "Metastream" target is used by all types of applications, including dev tools.
ly_create_alias(NAME Metastream.Clients  NAMESPACE Gem TARGETS Gem::Metastream)
ly_create_alias(NAME Metastream.Servers  NAMESPACE Gem TARGETS Gem::Metastream)
ly_create_alias(NAME Metastream.Builders NAMESPACE Gem TARGETS Gem::Metastream)
ly_create_alias(NAME Metastream.Tools    NAMESPACE Gem TARGETS Gem::Metastream)

################################################################################
# Tests
################################################################################
if(PAL_TRAIT_BUILD_TESTS_SUPPORTED)
    ly_add_target(
        NAME Metastream.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
        NAMESPACE Gem
        FILES_CMAKE
            metastream_tests_files.cmake
            metastream_shared_files.cmake
            ${pal_source_dir}/platform_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake
        PLATFORM_INCLUDE_FILES
            ${pal_source_dir}/metastream_${PAL_PLATFORM_NAME_LOWERCASE}.cmake
            ${common_source_dir}/${PAL_TRAIT_COMPILER_ID}/metastream_${PAL_TRAIT_COMPILER_ID_LOWERCASE}.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Tests
                Source
                ${pal_source_dir}
        BUILD_DEPENDENCIES
            PRIVATE
                AZ::AzTest
                Gem::Metastream.Static
                Legacy::CryCommon
    )
    ly_add_googletest(
        NAME Gem::Metastream.Tests
    )
endif()
