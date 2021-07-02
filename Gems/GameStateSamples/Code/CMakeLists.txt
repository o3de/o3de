#
# Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
# 
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

ly_get_list_relative_pal_filename(include_pal_dir ${CMAKE_CURRENT_LIST_DIR}/Include/Platform/${PAL_PLATFORM_NAME})

ly_add_target(
    NAME GameStateSamples.Headers HEADERONLY
    NAMESPACE Gem
    FILES_CMAKE
        gamestatesamples_headers_files.cmake
    INCLUDE_DIRECTORIES
        INTERFACE
            Include
            ${include_pal_dir}
    BUILD_DEPENDENCIES
        INTERFACE
            Gem::GameState
            Gem::LocalUser
            Gem::SaveData.Static
            Gem::MessagePopup.Static
            Legacy::CryCommon
)

ly_add_target(
    NAME GameStateSamples ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
    NAMESPACE Gem
    FILES_CMAKE
        gamestatesamples_shared_files.cmake
        ${include_pal_dir}/platform_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Source
    BUILD_DEPENDENCIES
        PRIVATE
            AZ::AzCore
            AZ::AzFramework
            Gem::LmbrCentral
            Gem::GameStateSamples.Headers
    RUNTIME_DEPENDENCIES
        Gem::GameState
        Gem::LocalUser
        Gem::LmbrCentral
)

# Clients and Servers use the above module.  There is no editor or tools module required.
ly_create_alias(NAME GameStateSamples.Clients  NAMESPACE Gem TARGETS GameStateSamples)
ly_create_alias(NAME GameStateSamples.Servers  NAMESPACE Gem TARGETS GameStateSamples)
