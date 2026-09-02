#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#

o3de_fetch_content(cityhash
    VERSION "1.1.1"
    LICENSE "MIT"
    URL "https://github.com/google/cityhash/archive/f5dc54147fcce12cefd16548c8e760d68ac04226.tar.gz" # 2022-07-19
    URL_HASH "22c24af73a3fec9402e3a183f2dee87d71f13a81859b0406472e671fb7d13a69"
    GIT "https://github.com/google/cityhash.git"
    GIT_HASH "f5dc54147fcce12cefd16548c8e760d68ac04226"
)
FetchContent_MakeAvailable(cityhash)

file(
    COPY
        "${cityhash_SOURCE_DIR}/src/city.h"
        "${cityhash_SOURCE_DIR}/src/citycrc.h"
    DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/Include"
)

include(CheckCXXSourceCompiles)
check_cxx_source_compiles(
    "int main() { return __builtin_expect(1, 1) ? 0 : 1; }"
    CITYHASH_HAVE_BUILTIN_EXPECT
)

include(TestBigEndian)
test_big_endian(CITYHASH_WORDS_BIGENDIAN)

configure_file(config.h.in "${CMAKE_CURRENT_BINARY_DIR}/config.h")

o3de_disable_warnings()

ly_add_target(
    NAME cityhash STATIC
    NAMESPACE 3rdParty
    NO_UNITY
    FILES_CMAKE files.cmake
)

target_include_directories(cityhash SYSTEM
    PRIVATE "${CMAKE_CURRENT_BINARY_DIR}"
    PUBLIC "${CMAKE_CURRENT_BINARY_DIR}/Include"
)

ly_install_files(
    FILES
        LICENSE.txt
        PackageInfo.json
    DESTINATION Code/3rdParty/cityhash
)
