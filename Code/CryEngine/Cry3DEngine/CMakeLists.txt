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

add_subdirectory(CGF)
add_subdirectory(MeshCompiler)

ly_get_list_relative_pal_filename(pal_dir ${CMAKE_CURRENT_LIST_DIR}/Platform/${PAL_PLATFORM_NAME})
ly_get_pal_tool_dirs(pal_tool_dirs ${CMAKE_CURRENT_LIST_DIR}/Platform)

ly_add_target(
    NAME Cry3DEngine.Static STATIC
    NAMESPACE Legacy
    FILES_CMAKE
        cry3dengine_files.cmake
        ${pal_dir}/platform_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake
    PLATFORM_INCLUDE_FILES
        ${pal_dir}/platform_${PAL_PLATFORM_NAME_LOWERCASE}.cmake
    INCLUDE_DIRECTORIES
        PUBLIC
            .
            ${pal_dir}
        PRIVATE
            ${pal_tool_dirs}
    BUILD_DEPENDENCIES
        PRIVATE
            3rdParty::mikkelsen
            Legacy::CryCommon
            Legacy::CryRender.Headers
)

ly_add_target(
    NAME Cry3DEngine ${PAL_TRAIT_MONOLITHIC_DRIVEN_LIBRARY_TYPE}
    NAMESPACE Legacy
    FILES_CMAKE
        cry3dengine_shared_files.cmake
    INCLUDE_DIRECTORIES
        PUBLIC
            .
            ..
    BUILD_DEPENDENCIES
        PRIVATE
            Legacy::Cry3DEngine.Static
            Legacy::Cry3DEngine.MeshCompiler.Static
            Legacy::Cry3DEngine.CGF.Static
            Legacy::CryCommon
)

################################################################################
# Tests
################################################################################
if(PAL_TRAIT_BUILD_TESTS_SUPPORTED)

    ly_add_target(
        NAME Cry3DEngine.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
        NAMESPACE Legacy
        FILES_CMAKE
            cry3dengine_test_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                .
        BUILD_DEPENDENCIES
            PRIVATE
                AZ::AzTest
                Legacy::CryCommon
                Legacy::Cry3DEngine.Static
                Legacy::Cry3DEngine.MeshCompiler.Static
                Legacy::Cry3DEngine.CGF.Static
    )
    ly_add_googletest(
        NAME Legacy::Cry3DEngine.Tests
    )
endif()
