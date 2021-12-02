#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

# PlatformTools enables to deal with tools that perform functionality cross-platform
# For example, the AssetProcessor can generate assets for other platforms. For the
# asset processor to be able to provide that, it requires to have the funcionality enabled.
# This cmake file provides an entry point to discover variables that will allow to enable
# the different platforms and variables that can be passed to enable those platforms to the
# code.

# Discover all the platforms that are available

set(LY_PAL_TOOLS_DEFINES)
file(GLOB pal_tools_files "cmake/Platform/*/PALTools_*.cmake")
foreach(pal_tools_file ${pal_tools_files})
    include(${pal_tools_file})
endforeach()
file(GLOB pal_restricted_tools_files "restricted/*/cmake/PALTools_*.cmake")
foreach(pal_restricted_tools_file ${pal_restricted_tools_files})
    include(${pal_restricted_tools_file})
endforeach()

# Set the AZ_TOOLS_EXPAND_FOR_RESTRICTED_PLATFORMS macro using the aggregate of all the LY_PAL_TOOLS_RESTRICTED_PLATFORM_DEFINES variable
if(LY_PAL_TOOLS_RESTRICTED_PLATFORM_DEFINES)
    list(JOIN LY_PAL_TOOLS_RESTRICTED_PLATFORM_DEFINES " " RESTRICTED_PLATFORM_DEFINES_MACRO)
    list(APPEND LY_PAL_TOOLS_DEFINES "AZ_TOOLS_EXPAND_FOR_RESTRICTED_PLATFORMS=${RESTRICTED_PLATFORM_DEFINES_MACRO}")
endif()
ly_set(LY_PAL_TOOLS_DEFINES ${LY_PAL_TOOLS_DEFINES})

# Include files to the CMakeFiles project
foreach(enabled_platform ${LY_PAL_TOOLS_ENABLED})
    string(TOLOWER ${enabled_platform} enabled_platform_lowercase)
    ly_get_absolute_pal_filename(pal_dir ${CMAKE_CURRENT_SOURCE_DIR}/cmake/Platform/${enabled_platform})
    ly_include_cmake_file_list(${pal_dir}/pal_tools_${enabled_platform_lowercase}_files.cmake)
endforeach()

function(ly_get_pal_tool_dirs out_list pal_path)
    set(pal_paths "")
    foreach(platform ${LY_PAL_TOOLS_ENABLED})
        ly_get_absolute_pal_filename(path ${pal_path}/${platform})
        list(APPEND pal_paths ${path})
    endforeach()
    set(${out_list} ${pal_paths} PARENT_SCOPE)
endfunction()
