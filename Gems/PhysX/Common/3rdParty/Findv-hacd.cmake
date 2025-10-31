#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

if (TARGET 3rdParty::v-hacd)
  return()
endif()

include(FetchContent)

FetchContent_Declare(
    v-hacd
    GIT_REPOSITORY https://github.com/kmammou/v-hacd.git
    GIT_TAG 22ec20a7f8ea221ab600df869369b9c6a258cb10
)

FetchContent_MakeAvailable(v-hacd)

FetchContent_GetProperties(v-hacd SOURCE_DIR V_HACD_SOURCE_DIR)
add_library(3rdParty::v-hacd IMPORTED INTERFACE GLOBAL ${V_HACD_SOURCE_DIR}/include/VHACD.h)
ly_target_include_system_directories(TARGET 3rdParty::v-hacd INTERFACE ${V_HACD_SOURCE_DIR}/include)

message(STATUS "PhysX gem uses v-hacd v4.1.0 (BSD-3-Clause) https://github.com/kmammou/v-hacd.git")

ly_install(FILES ${CMAKE_CURRENT_LIST_DIR}/Installer/Findv-hacd.cmake DESTINATION cmake/3rdParty)
set(v-hacd_FOUND TRUE)
