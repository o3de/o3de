#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

set(pybind11_FOUND TRUE)

if(TARGET 3rdParty::pybind11)
    return()
endif()

add_library(3rdParty::pybind11 INTERFACE IMPORTED GLOBAL)

if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(3rdParty::pybind11 INTERFACE -fsized-deallocation)
endif()

if(INSTALLED_ENGINE)
    ly_target_include_system_directories(
        TARGET 3rdParty::pybind11
        INTERFACE "${LY_ROOT_FOLDER}/include/pybind11"
    )
    return()
endif()

block()
    o3de_fetch_content(pybind11
        VERSION "v2.10.0"
        LICENSE "BSD-3-Clause"
        URL "https://github.com/pybind/pybind11/archive/refs/tags/v2.10.0.tar.gz"
        URL_HASH "eacf582fa8f696227988d08cfc46121770823839fe9e301a20fbce67e7cd70ec"
        GIT "https://github.com/pybind/pybind11.git"
        GIT_HASH "aa304c9c7d725ffb9d10af08a3b34cb372307020"
        SOURCE_SUBDIR "include"
    )

    FetchContent_MakeAvailable(pybind11)
    FetchContent_GetProperties(pybind11 SOURCE_DIR pybind11_source_dir)

    ly_target_include_system_directories(
        TARGET 3rdParty::pybind11
        INTERFACE "${pybind11_source_dir}/include"
    )

    ly_install(
        DIRECTORY "${pybind11_source_dir}/include/pybind11"
        DESTINATION include/pybind11
        COMPONENT CORE
    )
    ly_install(
        FILES "${pybind11_source_dir}/LICENSE"
        DESTINATION include/pybind11
        COMPONENT CORE
    )
endblock()
