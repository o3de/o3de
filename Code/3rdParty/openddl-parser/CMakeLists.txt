#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#

o3de_fetch_content(openddl-parser
    VERSION "0.5.2"
    LICENSE "MIT"
    URL "https://github.com/kimkulling/openddl-parser/archive/d1c739ab851b57af8237945fe4ffafc682c1dd2f.tar.gz"
    URL_HASH "f293e507cff47ad006e68eadaca5fccc862b551762034b7c8f4a1d4eb2830d64"
    GIT "https://github.com/kimkulling/openddl-parser.git"
    GIT_HASH "d1c739ab851b57af8237945fe4ffafc682c1dd2f"
    PATCH_FILES "${CMAKE_CURRENT_LIST_DIR}/openddl-parser.patch"
    NO_ADD_SUBDIR
)
FetchContent_MakeAvailable(openddl-parser)

file(COPY
    "${openddl-parser_SOURCE_DIR}/include/"
    DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/Include"
)

o3de_disable_warnings()

ly_add_target(
    NAME openddl-parser STATIC
    NAMESPACE 3rdParty
    NO_UNITY
    FILES_CMAKE files.cmake
    INCLUDE_DIRECTORIES
        PUBLIC "${CMAKE_CURRENT_BINARY_DIR}/Include"
    COMPILE_DEFINITIONS
        PUBLIC OPENDDL_STATIC_LIBARY
        PRIVATE OPENDDLPARSER_BUILD
)

set_target_properties(openddl-parser PROPERTIES
    POSITION_INDEPENDENT_CODE ON
    SYSTEM TRUE
)

target_compile_options(openddl-parser PRIVATE ${O3DE_COMPILE_OPTION_ENABLE_EXCEPTIONS})

ly_install_files(
    FILES
        LICENSE.txt
        PackageInfo.json
    DESTINATION Code/3rdParty/openddl-parser
)
