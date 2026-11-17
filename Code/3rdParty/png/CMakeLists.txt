#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#

o3de_fetch_content(png
    VERSION "1.6.59"
    LICENSE "libpng-2.0"
    URL "https://github.com/pnggroup/libpng/archive/refs/tags/v1.6.59.tar.gz"
    URL_HASH "2540302a1844ad2b2b501977abecfa850f265f97b78f065a712ab4074a89f5b5"
    GIT "https://github.com/pnggroup/libpng.git"
    GIT_HASH "cd952f49f95bb27154ae77dbb103032d95f6e580"
    NO_ADD_SUBDIR
)
FetchContent_MakeAvailable(png)

file(COPY
    "${png_SOURCE_DIR}/png.h"
    "${png_SOURCE_DIR}/pngconf.h"
    DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/Include"
)
configure_file(
    "${png_SOURCE_DIR}/scripts/pnglibconf.h.prebuilt"
    "${CMAKE_CURRENT_BINARY_DIR}/Include/pnglibconf.h"
    COPYONLY
)

set(png_arm_neon 0)
set(png_intel_sse 0)
string(TOLOWER "${CMAKE_SYSTEM_PROCESSOR}" png_processor)
if(png_processor MATCHES "^(arm64|aarch64)$")
    set(png_arm_neon 2)
elseif(png_processor MATCHES "^(x86_64|amd64)$")
    set(png_intel_sse 1)
endif()

o3de_disable_warnings()

ly_add_target(
    NAME png STATIC
    NAMESPACE 3rdParty
    NO_UNITY
    FILES_CMAKE files.cmake
    OUTPUT_NAME png16
    INCLUDE_DIRECTORIES
        PRIVATE "${png_SOURCE_DIR}"
        PUBLIC "${CMAKE_CURRENT_BINARY_DIR}/Include"
    COMPILE_DEFINITIONS
        PRIVATE
            PNG_ARM_NEON_OPT=${png_arm_neon}
            PNG_INTEL_SSE_OPT=${png_intel_sse}
)

find_package(ZLIB REQUIRED)
target_link_libraries(png PUBLIC ZLIB::ZLIB)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux" OR ANDROID)
    target_link_libraries(png PUBLIC m)
endif()

add_library(3rdParty::PNG ALIAS png)
add_library(PNG::PNG ALIAS png)

set_target_properties(png PROPERTIES
    POSITION_INDEPENDENT_CODE ON
    SYSTEM TRUE
)

ly_install_files(
    FILES
        LICENSE.txt
        PackageInfo.json
    DESTINATION Code/3rdParty/png
)
