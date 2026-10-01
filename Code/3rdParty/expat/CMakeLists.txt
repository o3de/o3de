#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#

o3de_fetch_content(expat
    VERSION "2.7.3"
    LICENSE "MIT"
    URL "https://github.com/libexpat/libexpat/archive/refs/tags/R_2_7_3.tar.gz"
    URL_HASH "7c853a76f7c2a562c3edfd84ca47e487d9eff3966786191dfb60cc441daf82d6"
    GIT "https://github.com/libexpat/libexpat.git"
    GIT_HASH "4575e52f83e0d6d7bd24939eab8952bbc7bc358f"
    NO_ADD_SUBDIR
)
FetchContent_MakeAvailable(expat)

file(COPY
    "${expat_SOURCE_DIR}/expat/lib/expat.h"
    "${expat_SOURCE_DIR}/expat/lib/expat_external.h"
    DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/Include"
)

include(CheckSymbolExists)
check_symbol_exists(arc4random_buf "stdlib.h" HAVE_ARC4RANDOM_BUF)
check_symbol_exists(getrandom "sys/random.h" HAVE_GETRANDOM)

if(CMAKE_C_BYTE_ORDER STREQUAL "BIG_ENDIAN")
    set(BYTEORDER 4321)
else()
    set(BYTEORDER 1234)
endif()

if(NOT WIN32 AND NOT HAVE_ARC4RANDOM_BUF AND NOT HAVE_GETRANDOM)
    set(XML_DEV_URANDOM 1)
endif()

set(XML_CONTEXT_BYTES 1024)
set(XML_DTD 1)
set(XML_GE 1)
set(XML_NS 1)

configure_file(
    "${expat_SOURCE_DIR}/expat/expat_config.h.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/Include/expat_config.h"
)

o3de_disable_warnings()

ly_add_target(
    NAME expat STATIC
    NAMESPACE 3rdParty
    NO_UNITY
    FILES_CMAKE files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE "${expat_SOURCE_DIR}/expat/lib"
        PUBLIC "${CMAKE_CURRENT_BINARY_DIR}/Include"
    COMPILE_DEFINITIONS
        PUBLIC XML_STATIC
)

set_target_properties(expat PROPERTIES
    POSITION_INDEPENDENT_CODE ON
    SYSTEM TRUE
)

ly_install_files(
    FILES
        LICENSE.txt
        PackageInfo.json
    DESTINATION Code/3rdParty/expat
)
