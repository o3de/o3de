#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#

o3de_fetch_content(clipper2
    VERSION "2.0.1"
    LICENSE "BSL-1.0"
    URL "https://github.com/PolyClipping/Clipper2/archive/refs/tags/Clipper2_2.0.1.tar.gz"
    URL_HASH "2a3693aceab4aed3e39b743e038d87701acc53cf05ed7b2013aab3e0aec5287e"
    GIT "https://github.com/PolyClipping/Clipper2.git"
    GIT_HASH "21ebba05db8894f0c7217ad35ea518080f324946"
    NO_ADD_SUBDIR
)
FetchContent_MakeAvailable(clipper2)

file(COPY
    "${clipper2_SOURCE_DIR}/CPP/Clipper2Lib/include/clipper2"
    DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/Include"
)

o3de_disable_warnings()

ly_add_target(
    NAME clipper2 STATIC
    NAMESPACE 3rdParty
    NO_UNITY
    FILES_CMAKE files.cmake
    INCLUDE_DIRECTORIES
        PUBLIC "${CMAKE_CURRENT_BINARY_DIR}/Include"
    COMPILE_DEFINITIONS
        PUBLIC CLIPPER2_MAX_DECIMAL_PRECISION=8
)

add_library(Clipper2::Clipper2 ALIAS clipper2)

set_target_properties(clipper2 PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    POSITION_INDEPENDENT_CODE ON
    SYSTEM TRUE
)

target_compile_options(clipper2 ${O3DE_COMPILE_OPTION_ENABLE_EXCEPTIONS})

ly_install_files(
    FILES
        LICENSE.txt
        PackageInfo.json
    DESTINATION Code/3rdParty/clipper2
)
