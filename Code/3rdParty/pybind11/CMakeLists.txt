#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#

o3de_fetch_content(pybind11
    VERSION "3.1.0"
    LICENSE "BSD-3-Clause"
    URL "https://github.com/pybind/pybind11/archive/refs/tags/v3.1.0.tar.gz"
    URL_HASH "ef712655692a2e9bf7bb7874c022564a45f91d847ddee987e720cd9e28849665"
    GIT "https://github.com/pybind/pybind11.git"
    GIT_HASH "97bf890db679505a14dfe547a5e77bb2bd05dc90"
    NO_ADD_SUBDIR
)
FetchContent_MakeAvailable(pybind11)

ly_add_target(
    NAME pybind11 HEADERONLY
    NAMESPACE 3rdParty
)

target_include_directories(pybind11 SYSTEM INTERFACE "${pybind11_SOURCE_DIR}/include")

if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(pybind11 INTERFACE -fsized-deallocation)
endif()

ly_install_files(
    FILES
        LICENSE.txt
        PackageInfo.json
    DESTINATION Code/3rdParty/pybind11
)
