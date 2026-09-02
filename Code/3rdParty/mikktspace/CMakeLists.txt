#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#

o3de_fetch_content(mikktspace
    VERSION "1.0"
    LICENSE "Zlib"
    URL "https://github.com/mmikk/MikkTSpace/archive/3e895b49d05ea07e4c2133156cfa94369e19e409.tar.gz"
    URL_HASH "aeba65ddee85a679133d510d71d00b108f603752f90b15b9ecf7777033f6e351"
    GIT "https://github.com/mmikk/MikkTSpace.git"
    GIT_HASH "3e895b49d05ea07e4c2133156cfa94369e19e409"
)
FetchContent_MakeAvailable(mikktspace)

file(COPY "${mikktspace_SOURCE_DIR}/mikktspace.h" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/Include")

o3de_disable_warnings()

ly_add_target(
    NAME mikktspace STATIC
    NAMESPACE 3rdParty
    NO_UNITY
    FILES_CMAKE files.cmake
)

target_include_directories(mikktspace SYSTEM PUBLIC "${CMAKE_CURRENT_BINARY_DIR}/Include")

if(UNIX)
    ly_target_link_libraries(mikktspace PUBLIC m)
endif()

ly_create_alias(NAME mikkelsen NAMESPACE 3rdParty TARGETS 3rdParty::mikktspace)

ly_install_files(
    FILES
        LICENSE.txt
        PackageInfo.json
    DESTINATION Code/3rdParty/mikktspace
)
