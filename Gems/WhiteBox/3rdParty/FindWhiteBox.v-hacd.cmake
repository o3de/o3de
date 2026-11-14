#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

# White Box owns its V-HACD target: in an installed SDK the PhysX gem defines 3rdParty::v-hacd as an empty stub
# (its code is prebuilt), so sharing that name would leave White Box without the header.
if (TARGET 3rdParty::WhiteBox.v-hacd)
    return()
endif()

include(FetchContent)

message(STATUS "WhiteBox Gem uses V-HACD v4.1.0 (BSD-3-Clause) https://github.com/kmammou/v-hacd")
message(STATUS "    - V-HACD splits concave White Box shells into convex collision hulls.")

# Header only: SOURCE_SUBDIR points at a folder without a CMakeLists.txt, so the download is not added as a project.
FetchContent_Declare(
    whitebox_v_hacd
    URL "https://github.com/kmammou/v-hacd/archive/refs/tags/v4.1.0.tar.gz"
    URL_HASH SHA256=9fe895cd10ec995d2171b11bde97aaaa221b418a3aaed0f5d9a068ae057d626b
    SOURCE_SUBDIR header_only_no_cmake
)
FetchContent_MakeAvailable(whitebox_v_hacd)
FetchContent_GetProperties(whitebox_v_hacd SOURCE_DIR WHITEBOX_V_HACD_SOURCE_DIR)

add_library(3rdParty::WhiteBox.v-hacd INTERFACE IMPORTED GLOBAL)
target_include_directories(3rdParty::WhiteBox.v-hacd SYSTEM INTERFACE ${WHITEBOX_V_HACD_SOURCE_DIR}/include)
