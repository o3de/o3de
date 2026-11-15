#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#

o3de_fetch_content(googletest
    VERSION "1.18.0"
    LICENSE "BSD-3-Clause"
    URL "https://github.com/google/googletest/archive/refs/tags/v1.18.0.tar.gz"
    URL_HASH "6e3191c1455468b3fc35a417fb565c1c5071aee1b7e7f85e30cf48a98d37d8b5"
    GIT "https://github.com/google/googletest.git"
    GIT_HASH "063de7e9578f82b369302001269680b4b1553359"
    NO_ADD_SUBDIR
)
FetchContent_MakeAvailable(googletest)

file(COPY
    "${googletest_SOURCE_DIR}/googletest/include/gtest"
    DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/Include"
)

file(COPY
    "${googletest_SOURCE_DIR}/googlemock/include/gmock"
    DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/Include"
)

find_package(Threads REQUIRED)

o3de_disable_warnings()

ly_add_target(
    NAME gtest STATIC
    NAMESPACE 3rdParty
    NO_UNITY
    FILES_CMAKE gtest_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE "${googletest_SOURCE_DIR}/googletest"
        PUBLIC "${CMAKE_CURRENT_BINARY_DIR}/Include"
    BUILD_DEPENDENCIES
        PUBLIC ${CMAKE_THREAD_LIBS_INIT}
)

ly_add_target(
    NAME gmock STATIC
    NAMESPACE 3rdParty
    NO_UNITY
    FILES_CMAKE gmock_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE "${googletest_SOURCE_DIR}/googlemock"
    BUILD_DEPENDENCIES
        PUBLIC 3rdParty::gtest
)

set_target_properties(gtest gmock PROPERTIES
    POSITION_INDEPENDENT_CODE ON
    SYSTEM TRUE
)

target_compile_options(gtest
    PUBLIC
        ${O3DE_COMPILE_OPTION_EXPORT_SYMBOLS}
        ${O3DE_TARGET_COMPILE_OPTION_DISABLE_FAST_MATH}
    PRIVATE
        ${O3DE_COMPILE_OPTION_ENABLE_EXCEPTIONS}
)

target_compile_options(gmock
    PUBLIC
        ${O3DE_COMPILE_OPTION_EXPORT_SYMBOLS}
        ${O3DE_TARGET_COMPILE_OPTION_DISABLE_FAST_MATH}
    PRIVATE
        ${O3DE_COMPILE_OPTION_ENABLE_EXCEPTIONS}
)

ly_create_alias(NAME GTest NAMESPACE 3rdParty::googletest TARGETS 3rdParty::gtest)
ly_create_alias(NAME GMock NAMESPACE 3rdParty::googletest TARGETS 3rdParty::gmock)

ly_install_files(
    FILES
        LICENSE.txt
        PackageInfo.json
    DESTINATION Code/3rdParty/googletest
)
