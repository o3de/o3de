#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#

o3de_fetch_content(freetype
    VERSION "2.14.3"
    LICENSE "FTL"
    URL "https://github.com/freetype/freetype/archive/refs/tags/VER-2-14-3.tar.gz"
    URL_HASH "dc49de6b01a266eef4876a4dd34d9842c475d3e28ff2eff63bd2fb760ab56261"
    GIT "https://github.com/freetype/freetype.git"
    GIT_HASH "c740f0fda4274d6ffd2e5b64a25b06ef69803a07"
)

set(BUILD_FRAMEWORK OFF)
set(BUILD_SHARED_LIBS OFF)
set(DISABLE_FORCE_DEBUG_POSTFIX ON)
set(FT_DISABLE_BROTLI ON)
set(FT_DISABLE_BZIP2 ON)
set(FT_DISABLE_HARFBUZZ ON)
set(FT_DISABLE_PNG ON)
set(FT_DISABLE_ZLIB OFF)
set(FT_REQUIRE_ZLIB ON)
set(SKIP_INSTALL_ALL ON)

FetchContent_MakeAvailable(freetype)

set_target_properties(freetype PROPERTIES
    POSITION_INDEPENDENT_CODE ON
    SYSTEM TRUE
)

o3de_fixup_fetchcontent_targets(TARGETS freetype)

add_library(3rdParty::Freetype ALIAS freetype)
add_library(Freetype::Freetype ALIAS freetype)

ly_install(
    DIRECTORY
        "${freetype_SOURCE_DIR}/include/"
        "${freetype_BINARY_DIR}/include/"
    DESTINATION include/freetype2
    COMPONENT ${CMAKE_INSTALL_DEFAULT_COMPONENT_NAME}
)

ly_install_files(
    FILES
        LICENSE.txt
        PackageInfo.json
    DESTINATION Code/3rdParty/freetype
)
