#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

o3de_pal_dir(pal_dir ${CMAKE_CURRENT_LIST_DIR}/Platform/${PAL_PLATFORM_NAME} ${O3DE_ENGINE_RESTRICTED_PATH} ${LY_ROOT_FOLDER})

add_subdirectory(XML)

ly_add_target(
    NAME CrySystem.Static STATIC
    NAMESPACE Legacy
    FILES_CMAKE
        crysystem_files.cmake
        ${pal_dir}/platform_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake # Required for restricted platforms
    PLATFORM_INCLUDE_FILES
        ${pal_dir}/platform_${PAL_PLATFORM_NAME_LOWERCASE}.cmake # Required for restricted platforms
    INCLUDE_DIRECTORIES
        PUBLIC
            .
            ${pal_dir} # Required for restricted platforms
    BUILD_DEPENDENCIES
        PRIVATE
            3rdParty::expat
            3rdParty::lz4
            3rdParty::TIFF
            3rdParty::zstd
            Legacy::CryCommon
            Legacy::CrySystem.XMLBinary
            Legacy::RemoteConsoleCore
            AZ::AzFramework
        PUBLIC
            Gem::AudioSystem.API
)

ly_add_source_properties(
    SOURCES SystemInit.cpp
    PROPERTY COMPILE_DEFINITIONS 
    VALUES ${LY_PAL_TOOLS_DEFINES}
)

ly_add_target(
    NAME CrySystem ${PAL_TRAIT_MONOLITHIC_DRIVEN_LIBRARY_TYPE}
    NAMESPACE Legacy
    FILES_CMAKE
        crysystem_shared_files.cmake
    INCLUDE_DIRECTORIES
        PUBLIC
            .
    BUILD_DEPENDENCIES
        PRIVATE
            Legacy::CrySystem.Static
            AZ::AzCore
            Legacy::CryCommon
)
