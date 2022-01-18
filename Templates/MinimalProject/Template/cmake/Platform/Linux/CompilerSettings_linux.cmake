# {BEGIN_LICENSE}
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
# {END_LICENSE}

if(NOT CMAKE_C_COMPILER AND NOT CMAKE_CXX_COMPILER AND NOT "$ENV{CC}" AND NOT "$ENV{CXX}")
    set(path_search
        /bin
        /usr/bin
        /usr/local/bin
        /sbin
        /usr/sbin
        /usr/local/sbin
    )
    list(TRANSFORM path_search APPEND "/clang-[0-9]*")
    file(GLOB clang_versions ${path_search})
    if(clang_versions)
        # Find and pick the highest installed version
        list(SORT clang_versions COMPARE NATURAL)
        list(GET clang_versions 0 clang_higher_version_path)
        string(REGEX MATCH "clang-([0-9.]*)" clang_higher_version ${clang_higher_version_path})
        if(CMAKE_MATCH_1)
            set(CMAKE_C_COMPILER clang-${CMAKE_MATCH_1})
            set(CMAKE_CXX_COMPILER clang++-${CMAKE_MATCH_1})
        else()
            message(FATAL_ERROR "Clang not found, please install clang")
        endif()
    else()
        message(FATAL_ERROR "Clang not found, please install clang")
    endif()
endif()
