#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#

o3de_fetch_content(mcpp
    VERSION "2.7.2"
    LICENSE "BSD-2-Clause AND (Apache-2.0 OR MIT)"
    URL "https://github.com/EnterTheArcane/mcpp/archive/c8da19d04e248b129bfd90a4fd7882585148e4e5.tar.gz"
    URL_HASH "b5b4face7d46a0ba74f97680814c1e714d2b729db74961b89dadc59e97c3c48d"
    GIT "https://github.com/EnterTheArcane/mcpp.git"
    GIT_HASH "c8da19d04e248b129bfd90a4fd7882585148e4e5"
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
