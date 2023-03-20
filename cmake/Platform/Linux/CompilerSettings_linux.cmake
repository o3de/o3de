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
    unset(compiler_found)
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
                set(compiler_found TRUE)
            elseif(EXISTS "${clang_path_prefix}++")
                set(CMAKE_C_COMPILER ${clang_higher_version_path})
                set(CMAKE_CXX_COMPILER ${clang_path_prefix}++)
                set(compiler_found TRUE)
            endif()
        endif()
    endif()
    # If clang-<version> number could not be found, try seaching for a clang executable without the version suffix
    if(NOT compiler_found)
        # Look for clang without a version idenfiier
        list(TRANSFORM path_search APPEND "/clang" OUTPUT_VARIABLE path_no_version_search)
        file(GLOB clang_executables ${path_no_version_search})
        if(clang_executables)
            list(GET clang_executables 0 clang_executable_front)
            cmake_path(GET clang_executable_front PARENT_PATH clang_cplusplus_executable)
            cmake_path(APPEND clang_cplusplus_executable "clang++")
            if(EXISTS "${clang_executable_front}" AND EXISTS "${clang_cplusplus_executable}")
                set(CMAKE_C_COMPILER ${clang_executable_front})
                set(CMAKE_CXX_COMPILER ${clang_cplusplus_executable})
                set(compiler_found TRUE)
            endif()
        endif()
    endif()
    if(NOT compiler_found)
        # Look for gcc without a version idenfiier
        list(TRANSFORM path_search APPEND "/gcc" OUTPUT_VARIABLE path_no_version_search)
        file(GLOB gcc_executables ${path_no_version_search})
        message(STATUS ${gcc_executables})
        if(gcc_executables)
            # Grab the path to the first gcc executable found
            list(GET gcc_executables 0 gcc_executable_front)
            # Make sure that the GNU g++ frontend exist as well
            cmake_path(GET gcc_executable_front PARENT_PATH gnu_cplusplus_executable)
            cmake_path(APPEND gnu_cplusplus_executable "g++")
            if(EXISTS "${gcc_executable_front}" AND EXISTS "${gnu_cplusplus_executable}")
                set(CMAKE_C_COMPILER ${gcc_executable_front})
                set(CMAKE_CXX_COMPILER ${gnu_cplusplus_executable})
                set(compiler_found TRUE)
            endif()
        endif()
    endif()
    if(NOT compiler_found)
        message(FATAL_ERROR "Neither Clang nor GCC has been found, please install clang or gcc or specify"
        " the compilers to use in CMAKE_C_COMPILER and CMAKE_CXX_COMPILER")
    endif()
endif()
