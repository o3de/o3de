#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#

o3de_fetch_content(libsamplerate
    VERSION "0.2.2"
    LICENSE "BSD-2-Clause"
    URL "https://github.com/libsndfile/libsamplerate/archive/refs/tags/0.2.2.tar.gz"
    URL_HASH "16e881487f184250deb4fcb60432d7556ab12cb58caea71ef23960aec6c0405a"
    GIT "https://github.com/libsndfile/libsamplerate.git"
    GIT_HASH "c96f5e3de9c4488f4e6c97f59f5245f22fda22f7"
    NO_ADD_SUBDIR
)
FetchContent_MakeAvailable(libsamplerate)

file(
    COPY "${libsamplerate_SOURCE_DIR}/include/samplerate.h"
    DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/Include"
)

o3de_disable_warnings()

ly_add_target(
    NAME libsamplerate STATIC
    NAMESPACE 3rdParty
    NO_UNITY
    FILES_CMAKE files.cmake
    OUTPUT_NAME samplerate
    INCLUDE_DIRECTORIES
        PRIVATE "${libsamplerate_SOURCE_DIR}/src"
        PUBLIC "${CMAKE_CURRENT_BINARY_DIR}/Include"
    COMPILE_DEFINITIONS
        PRIVATE
            PACKAGE=\"libsamplerate\"
            VERSION=\"0.2.2\"
            HAVE_STDBOOL_H
            ENABLE_SINC_FAST_CONVERTER
            ENABLE_SINC_MEDIUM_CONVERTER
            ENABLE_SINC_BEST_CONVERTER
)

set_target_properties(libsamplerate PROPERTIES
    C_STANDARD 99
    POSITION_INDEPENDENT_CODE ON
    SYSTEM TRUE
)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux" OR ANDROID)
    target_link_libraries(libsamplerate PRIVATE m)
endif()

ly_install_files(
    FILES
        LICENSE.txt
        PackageInfo.json
    DESTINATION Code/3rdParty/libsamplerate
)
