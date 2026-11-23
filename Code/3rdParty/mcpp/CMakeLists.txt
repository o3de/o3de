#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#

o3de_fetch_content(mcpp
    VERSION "2.7.2"
    LICENSE "BSD-2-Clause AND (Apache-2.0 OR MIT)"
    URL "https://github.com/o3de/mcpp/archive/ea985ce7beb7d3f750f1a7a3f452568bae06b2e7.tar.gz"
    URL_HASH "1129651b911761c137f720ef574fc65d9e8b63556db056c6b40d85426f285213"
    GIT "https://github.com/o3de/mcpp.git"
    GIT_HASH "ea985ce7beb7d3f750f1a7a3f452568bae06b2e7"
    NO_ADD_SUBDIR
)
FetchContent_MakeAvailable(mcpp)

o3de_disable_warnings()

ly_add_target(
    NAME mcpp STATIC
    NAMESPACE 3rdParty
    NO_UNITY
    FILES_CMAKE files.cmake
    INCLUDE_DIRECTORIES
        PUBLIC "${mcpp_SOURCE_DIR}/include"
    COMPILE_DEFINITIONS
        PRIVATE
            MCPP_LIB=1
)

set_target_properties(mcpp PROPERTIES SYSTEM TRUE)

ly_install_files(
    FILES
        LICENSE.txt
        PackageInfo.json
    DESTINATION Code/3rdParty/mcpp
)
