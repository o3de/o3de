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
ly_get_pal_tool_dirs(pal_tool_dirs ${CMAKE_CURRENT_LIST_DIR}/Platform)

ly_add_target(
    NAME CryCommon STATIC
    NAMESPACE Legacy
    FILES_CMAKE
        crycommon_files.cmake
        ${pal_dir}/crycommon_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake
    PLATFORM_INCLUDE_FILES
        ${pal_dir}/crycommon_${PAL_PLATFORM_NAME_LOWERCASE}.cmake
    INCLUDE_DIRECTORIES
        PUBLIC
            .         # Lots of code without CryCommon/
            ..        # Dangerous since exports CryEngine's path (client code can do CrySystem/ without depending on that target)
            ${pal_dir}
            ${pal_tool_dirs}
    BUILD_DEPENDENCIES
        PUBLIC
            AZ::AzCore
            AZ::AzFramework
)

if(PAL_TRAIT_BUILD_TESTS_SUPPORTED)

    ly_add_target(
        NAME CryCommonMocks HEADERONLY
        NAMESPACE Legacy
        FILES_CMAKE
            crycommon_testing_files.cmake
        INCLUDE_DIRECTORIES
            INTERFACE
                Mocks
    )

endif()
