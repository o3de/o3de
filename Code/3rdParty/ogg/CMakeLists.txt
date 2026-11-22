#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#

o3de_fetch_content(ogg
    VERSION "1.3.6"
    LICENSE "BSD-3-Clause"
    URL "https://github.com/xiph/ogg/archive/refs/tags/v1.3.6.tar.gz"
    URL_HASH "95b643da661155d79db9de2fca55daed3a8d491039829def246aacb3d9201c81"
    GIT "https://github.com/xiph/ogg.git"
    GIT_HASH "be05b13e98b048f0b5a0f5fa8ce514d56db5f822"
    NO_ADD_SUBDIR
)
FetchContent_MakeAvailable(ogg)

file(COPY
    "${ogg_SOURCE_DIR}/include/ogg/ogg.h"
    "${ogg_SOURCE_DIR}/include/ogg/os_types.h"
    DESTINATION "${CMAKE_BINARY_DIR}/include/ogg/ogg"
)

set(INCLUDE_INTTYPES_H 0)
set(INCLUDE_STDINT_H 1)
set(INCLUDE_SYS_TYPES_H 0)
set(SIZE16 int16_t)
set(USIZE16 uint16_t)
set(SIZE32 int32_t)
set(USIZE32 uint32_t)
set(SIZE64 int64_t)
set(USIZE64 uint64_t)
configure_file(
    "${ogg_SOURCE_DIR}/include/ogg/config_types.h.in"
    "${CMAKE_BINARY_DIR}/include/ogg/ogg/config_types.h"
    @ONLY
)

o3de_disable_warnings()

ly_add_target(
    NAME ogg STATIC
    NAMESPACE 3rdParty
    FILES_CMAKE files.cmake
    INCLUDE_DIRECTORIES
        PUBLIC "${CMAKE_BINARY_DIR}/include/ogg"
)

add_library(Ogg::ogg ALIAS ogg)

set_target_properties(ogg PROPERTIES
    POSITION_INDEPENDENT_CODE ON
    SYSTEM TRUE
)

ly_install_files(
    FILES
        LICENSE.txt
        PackageInfo.json
    DESTINATION Code/3rdParty/ogg
)
