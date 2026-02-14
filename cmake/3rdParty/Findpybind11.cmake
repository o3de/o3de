#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

include_guard(GLOBAL)

function(_pybind11_scope)
    set(package_name "pybind11")
    set(package_version "2.10.0")
    set(package_hash "SHA256=eacf582fa8f696227988d08cfc46121770823839fe9e301a20fbce67e7cd70ec")

    if(TARGET 3rdParty::${package_name})
        return()
    endif()

    set(CMAKE_POLICY_VERSION_MINIMUM "3.5")
    FetchContent_Declare(${package_name}
        URL "https://github.com/pybind/pybind11/archive/refs/tags/v${package_version}.tar.gz"
        URL_HASH ${package_hash}

        DOWNLOAD_DIR "${LY_PACKAGE_DOWNLOAD_CACHE_LOCATION}/${package_name}"
        DOWNLOAD_NO_PROGRESS TRUE

        EXCLUDE_FROM_ALL

        CMAKE_ARGS
        -DPYBIND11_INSTALL=OFF
        -DPYBIND11_TEST=OFF
    )
    FetchContent_MakeAvailable(${package_name})

    add_library(3rdParty::${package_name} INTERFACE IMPORTED GLOBAL)
    target_link_libraries(3rdParty::${package_name} INTERFACE pybind11::pybind11)

    if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        target_compile_options(3rdParty::${package_name} INTERFACE -fsized-deallocation)
    endif()

    message(STATUS "Using ${package_name}@${package_version} (BSD-3-Clause)")
endfunction()

_pybind11_scope()
