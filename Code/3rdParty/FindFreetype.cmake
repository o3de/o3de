#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#

if(INSTALLED_ENGINE)
    if(NOT TARGET Freetype::Freetype)
        add_library(Freetype::Freetype STATIC IMPORTED GLOBAL)
        set(freetype_library_dir "${LY_ROOT_FOLDER}/lib/${PAL_PLATFORM_NAME}")
        set_target_properties(Freetype::Freetype PROPERTIES
            IMPORTED_LOCATION "${freetype_library_dir}/profile/${CMAKE_STATIC_LIBRARY_PREFIX}freetype${CMAKE_STATIC_LIBRARY_SUFFIX}"
            IMPORTED_LOCATION_DEBUG "${freetype_library_dir}/debug/${CMAKE_STATIC_LIBRARY_PREFIX}freetype${CMAKE_STATIC_LIBRARY_SUFFIX}"
            IMPORTED_LOCATION_RELEASE "${freetype_library_dir}/release/${CMAKE_STATIC_LIBRARY_PREFIX}freetype${CMAKE_STATIC_LIBRARY_SUFFIX}"
        )
        target_include_directories(Freetype::Freetype SYSTEM INTERFACE "${LY_ROOT_FOLDER}/include/freetype2")
        if(NOT TARGET ZLIB::ZLIB)
            find_package(ZLIB REQUIRED)
        endif()
        target_link_libraries(Freetype::Freetype INTERFACE ZLIB::ZLIB)
        add_library(3rdParty::Freetype ALIAS Freetype::Freetype)
    endif()
else()
    o3de_resolve_3rdparty_target(3rdParty::Freetype freetype_resolved)
endif()

get_target_property(FREETYPE_INCLUDE_DIRS 3rdParty::Freetype INTERFACE_INCLUDE_DIRECTORIES)
set(FREETYPE_LIBRARIES Freetype::Freetype)
set(Freetype_VERSION "2.14.3")
set(FREETYPE_VERSION_STRING "${Freetype_VERSION}")

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Freetype
    REQUIRED_VARS FREETYPE_LIBRARIES FREETYPE_INCLUDE_DIRS
    VERSION_VAR Freetype_VERSION
)
