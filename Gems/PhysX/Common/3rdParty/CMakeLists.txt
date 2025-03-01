#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

include(FetchContent)

FetchContent_Declare(
    v-hacd
    URL          https://github.com/kmammou/v-hacd/raw/refs/tags/v4.1.0/include/VHACD.h
    URL_HASH     MD5=19397dad8ffed62b1c6a24c404a3cd41
    DOWNLOAD_NO_EXTRACT TRUE
    DOWNLOAD_NO_PROGRESS TRUE
    TLS_VERIFY TRUE
)

FetchContent_MakeAvailable(v-hacd)

FetchContent_GetProperties(v-hacd SOURCE_DIR V_HACD_SOURCE_DIR)
add_library(3rdParty::v-hacd IMPORTED INTERFACE GLOBAL ${V_HACD_SOURCE_DIR}/VHACD.h)
target_include_directories(3rdParty::v-hacd SYSTEM INTERFACE ${V_HACD_SOURCE_DIR})

message(STATUS "Using v-hacd ${V_HACD_SOURCE_DIR}/VHACD.h - see ${CMAKE_CURRENT_LIST_DIR}/v-hacd/LICENSE")