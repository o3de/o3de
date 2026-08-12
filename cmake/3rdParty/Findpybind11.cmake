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
    ly_target_include_system_directories(TARGET 3rdParty::pybind11
        INTERFACE "${LY_ROOT_FOLDER}/include/pybind11"
    )
    return()
endif()

block()
    set(ADDITIONAL_FETCHCONTENT_FLAGS "")
    if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.25")
        list(APPEND ADDITIONAL_FETCHCONTENT_FLAGS "SYSTEM")
    endif()

    o3de_fetch_content(pybind11
        VERSION "v2.10.0"
        LICENSE "BSD-3-Clause"
        URL "https://github.com/pybind/pybind11/archive/refs/tags/v2.10.0.tar.gz"
        URL_HASH "eacf582fa8f696227988d08cfc46121770823839fe9e301a20fbce67e7cd70ec"
        GIT "https://github.com/pybind/pybind11.git"
        GIT_HASH "aa304c9c7d725ffb9d10af08a3b34cb372307020"
        EXCLUDE_FROM_ALL
        ${ADDITIONAL_FETCHCONTENT_FLAGS}
    )

    # FetchContent ignores CMAKE_ARGS, so configure pybind11 through the variables its option() calls consume.
    # O3DE provides Python separately through 3rdParty::Python; only pybind11's headers are needed here.
    set(PYBIND11_INSTALL OFF)
    set(PYBIND11_NOPYTHON ON)
    set(PYBIND11_TEST OFF)
    set(CMAKE_POLICY_VERSION_MINIMUM "3.5")

    set(OLD_LOG_LEVEL ${CMAKE_MESSAGE_LOG_LEVEL})
    set(CMAKE_MESSAGE_LOG_LEVEL ${O3DE_FETCHCONTENT_MESSAGE_LEVEL})

    FetchContent_MakeAvailable(pybind11)

    set(CMAKE_MESSAGE_LOG_LEVEL ${OLD_LOG_LEVEL})

    target_link_libraries(3rdParty::pybind11 INTERFACE pybind11::pybind11)

    FetchContent_GetProperties(pybind11 SOURCE_DIR pybind11_source_dir)
    ly_install(DIRECTORY "${pybind11_source_dir}/include/pybind11"
        DESTINATION include/pybind11
        COMPONENT CORE
    )
    ly_install(FILES "${pybind11_source_dir}/LICENSE"
        DESTINATION include/pybind11
        COMPONENT CORE
    )
endblock()
