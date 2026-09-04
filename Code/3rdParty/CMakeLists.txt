#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#

file(
    GLOB third_party_cmake_lists
    LIST_DIRECTORIES FALSE
    RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
    CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*/CMakeLists.txt"
)
list(
    SORT third_party_cmake_lists
    COMPARE NATURAL
    CASE INSENSITIVE
    ORDER ASCENDING
)

foreach(third_party_cmake_list IN LISTS third_party_cmake_lists)
    cmake_path(GET third_party_cmake_list PARENT_PATH third_party_directory)
    add_subdirectory("${third_party_directory}")
endforeach()
