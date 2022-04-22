#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

if(NOT CMAKE_C_COMPILER AND NOT CMAKE_CXX_COMPILER AND NOT "$ENV{CC}" AND NOT "$ENV{CXX}")
    set(path_search
        /bin
        /usr/bin
        /usr/local/bin
        /sbin
        /usr/sbin
        /usr/local/sbin
    )
    list(TRANSFORM path_search APPEND "/clang-[0-9]*" OUTPUT_VARIABLE path_with_version_search)
    file(GLOB clang_versions ${path_with_version_search})
    unset(clang_found)
    # First search for clang with a version value
    if(clang_versions)
        # Find and pick the highest installed version
        list(SORT clang_versions COMPARE NATURAL ORDER DESCENDING)
        list(GET clang_versions 0 clang_higher_version_path)
        string(REGEX MATCH "(.*clang)-([0-9.]*)" clang_higher_version ${clang_higher_version_path})
        if(CMAKE_MATCH_2 AND EXISTS "${clang_higher_version_path}")
            set(clang_path_prefix ${CMAKE_MATCH_1})
            set(clang_version_suffix ${CMAKE_MATCH_2})
            if(EXISTS "${clang_path_prefix}++-${clang_version_suffix}")
                set(CMAKE_C_COMPILER ${clang_higher_version_path})
                set(CMAKE_CXX_COMPILER ${clang_path_prefix}++-${clang_version_suffix})
                set(clang_found TRUE)
            elseif(EXISTS ":${clang_path_prefix}++")
                set(CMAKE_C_COMPILER ${clang_higher_version_path})
                set(CMAKE_CXX_COMPILER ${clang_path_prefix}++)
                set(clang_found TRUE)
            endif()
        endif()
    endif()
    # If clang-<version> number could not be found, try seaching for a clang executable without the version suffix
    if(NOT clang_found)
        # Look for clang without a version idenfiier
        list(TRANSFORM path_search APPEND "/clang" OUTPUT_VARIABLE path_no_version_search)
        file(GLOB clang_executables ${path_no_version_search})
        if(clang_executables)
            list(GET clang_executables 0 clang_executable_front)
            if(EXISTS "${clang_executable_front}" AND EXISTS "${clang_executable_front}++")
                set(CMAKE_C_COMPILER ${clang_executable_front})
                set(CMAKE_CXX_COMPILER ${clang_executable_front}++)
                set(clang_found TRUE)
            endif()
        endif()
    endif()
    if(NOT clang_found)
        message(FATAL_ERROR "Clang not found, please install clang or specify compilers to use in CMAKE_C_COMPILER and CMAKE_CXX_COMPILER")
    endif()
endif()
