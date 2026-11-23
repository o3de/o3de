#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

if (NOT PAL_TRAIT_BUILD_HOST_TOOLS)
    return()
endif()

set(sdkwrapper_dir ${CMAKE_CURRENT_LIST_DIR}/../SDKWrapper)

ly_add_target(
    NAME SDKWrapper STATIC
    NAMESPACE AZ
    FILES_CMAKE
        sdkwrapper_files.cmake
    INCLUDE_DIRECTORIES
        PUBLIC
            .
            ../..
    BUILD_DEPENDENCIES
        PRIVATE
            AZ::AzCore
            AZ::AzToolsFramework
        PUBLIC
            3rdParty::assimp
)
