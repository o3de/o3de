#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

o3de_pal_dir(pal_dir ${CMAKE_CURRENT_LIST_DIR}/Platform/${PAL_PLATFORM_NAME} ${O3DE_ENGINE_RESTRICTED_PATH} ${LY_ROOT_FOLDER})

include(${pal_dir}/PAL_${PAL_PLATFORM_NAME_LOWERCASE}.cmake)

if(NOT PAL_TRAIT_BUILD_CRASH_HANDLER_SUPPORTED)
    return()
endif()

add_subdirectory(Support)

ly_add_target(
    NAME CrashHandler STATIC
    NAMESPACE AZ
    FILES_CMAKE
        crash_handler_files.cmake
        ${pal_dir}/crash_handler_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake
        ${pal_dir}/platform_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake
    INCLUDE_DIRECTORIES
        PUBLIC
            Shared
        PRIVATE
            ${pal_dir}
    BUILD_DEPENDENCIES
        PUBLIC
            AZ::CrashSupport
        PRIVATE
            3rdParty::Crashpad
            AZ::AzFramework
)

string(REPLACE "." ";" version_list "${O3DE_INSTALL_VERSION_STRING}")
list(GET version_list 0 EXE_VERSION_INFO_0)
list(GET version_list 1 EXE_VERSION_INFO_1)

list(LENGTH version_list version_component_count)
if(${version_component_count} GREATER_EQUAL 3)
    list(GET version_list 2 EXE_VERSION_INFO_2)
else()
    set(EXE_VERSION_INFO_2 0)
endif()

if(${version_component_count} GREATER_EQUAL 4)
    list(GET version_list 3 EXE_VERSION_INFO_3)
else()
    set(EXE_VERSION_INFO_3 0)
endif()

ly_add_source_properties(
    SOURCES Shared/CrashHandler.cpp
    PROPERTY COMPILE_DEFINITIONS 
    VALUES
        LY_BUILD=${O3DE_INSTALL_BUILD_VERSION}
        EXE_VERSION_INFO_0=${EXE_VERSION_INFO_0}
        EXE_VERSION_INFO_1=${EXE_VERSION_INFO_1}
        EXE_VERSION_INFO_2=${EXE_VERSION_INFO_2}
        EXE_VERSION_INFO_3=${EXE_VERSION_INFO_3}
)

ly_add_target(
    NAME CrashUploaderSupport STATIC
    NAMESPACE AZ
    FILES_CMAKE
        crash_uploader_support_files.cmake
    INCLUDE_DIRECTORIES
        PUBLIC
            Uploader/include
        PRIVATE
            Uploader/include/Uploader
    BUILD_DEPENDENCIES
        PUBLIC
            3rdParty::Crashpad::Handler
            AZ::CrashSupport
)

add_subdirectory(Tools)
