#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#

o3de_fetch_content(expat
    VERSION "2.10.0"
    LICENSE "MIT"
    URL "https://github.com/libexpat/libexpat/archive/refs/tags/R_2_10_0.tar.gz"
    URL_HASH "b3644a12f62d7762b62c444f69d169ba0234f13138d0240244bca64cbe35286f"
    GIT "https://github.com/libexpat/libexpat.git"
    GIT_HASH "a70841d9ffa64ddde29ab824ed6305035ebe4c47"
    NO_ADD_SUBDIR
)
FetchContent_MakeAvailable(expat)

include("${expat_SOURCE_DIR}/expat/ConfigureChecks.cmake")

set(PACKAGE_NAME expat)
set(PACKAGE_VERSION 2.10.0)
set(PACKAGE_STRING "${PACKAGE_NAME} ${PACKAGE_VERSION}")
set(PACKAGE_TARNAME "${PACKAGE_NAME}")
set(PACKAGE_BUGREPORT "https://github.com/libexpat/libexpat/issues")
set(XML_CONTEXT_BYTES 1024)
set(XML_DTD 1)
set(XML_GE 1)
set(XML_NS 1)
set(XML_DEV_URANDOM 1)

configure_file(
    "${expat_SOURCE_DIR}/expat/expat_config.h.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/Include/expat_config.h"
)

file(COPY
    "${expat_SOURCE_DIR}/expat/lib/expat.h"
    "${expat_SOURCE_DIR}/expat/lib/expat_external.h"
    DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/Include"
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
    BUILD_DEPENDENCIES
        PUBLIC $<$<BOOL:${_EXPAT_LIBM_FOUND}>:m>
)

set_target_properties(expat PROPERTIES
    C_STANDARD 99
    C_STANDARD_REQUIRED ON
    C_VISIBILITY_PRESET hidden
    POSITION_INDEPENDENT_CODE ON
    SYSTEM TRUE
)

add_library(expat::expat ALIAS expat)

ly_install_files(
    FILES
        LICENSE.txt
        PackageInfo.json
    DESTINATION Code/3rdParty/expat
)
