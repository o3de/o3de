#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#

o3de_fetch_content(vorbis
    VERSION "1.3.7"
    LICENSE "BSD-3-Clause"
    URL "https://github.com/xiph/vorbis/archive/refs/tags/v1.3.7.tar.gz"
    URL_HASH "270c76933d0934e42c5ee0a54a36280e2d87af1de3cc3e584806357e237afd13"
    GIT "https://github.com/xiph/vorbis.git"
    GIT_HASH "0657aee69dec8508a0011f47f3b69d7538e9d262"
    NO_ADD_SUBDIR
)
FetchContent_MakeAvailable(vorbis)

file(COPY
    "${vorbis_SOURCE_DIR}/include/vorbis"
    DESTINATION "${CMAKE_BINARY_DIR}/include/vorbis"
)

o3de_disable_warnings()

ly_add_target(
    NAME vorbis STATIC
    NAMESPACE 3rdParty
    FILES_CMAKE files.cmake
    INCLUDE_DIRECTORIES
        PUBLIC "${CMAKE_BINARY_DIR}/include/vorbis"
        PRIVATE "${vorbis_SOURCE_DIR}/lib"
    BUILD_DEPENDENCIES
        PUBLIC 3rdParty::ogg
)

set_target_properties(vorbis PROPERTIES
    POSITION_INDEPENDENT_CODE ON
    SYSTEM TRUE
    UNITY_BUILD_BATCH_SIZE 8
)

if(UNIX)
    ly_target_link_libraries(vorbis PRIVATE m)
endif()

ly_install_files(
    FILES
        LICENSE.txt
        PackageInfo.json
    DESTINATION Code/3rdParty/vorbis
)
