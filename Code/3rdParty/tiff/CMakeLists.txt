#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#

o3de_fetch_content(tiff
    VERSION "4.7.2"
    LICENSE "libtiff"
    URL "https://gitlab.com/libtiff/libtiff/-/archive/v4.7.2/libtiff-v4.7.2.tar.gz"
    URL_HASH "62b15f44c10106065211efa22a2dc77937bd97ee1f6e3afd2c9659010de112eb"
    GIT "https://gitlab.com/libtiff/libtiff.git"
    GIT_HASH "d01a94be176f5f6a87f7ee1c0b32e65416aa2b4d"
    NO_ADD_SUBDIR
)
FetchContent_MakeAvailable(tiff)

include("${tiff_SOURCE_DIR}/cmake/ProcessorChecks.cmake")
include("${tiff_SOURCE_DIR}/cmake/IncludeChecks.cmake")
include("${tiff_SOURCE_DIR}/cmake/SymbolChecks.cmake")
include("${tiff_SOURCE_DIR}/cmake/TypeSizeChecks.cmake")

set(LIBTIFF_VERSION "4.7.2")
set(LIBTIFF_MAJOR_VERSION 4)
set(LIBTIFF_MINOR_VERSION 7)
set(LIBTIFF_MICRO_VERSION 2)
file(STRINGS "${tiff_SOURCE_DIR}/RELEASE-DATE" LIBTIFF_RELEASE_DATE)
set(PACKAGE_NAME "LibTIFF Software")
set(PACKAGE_TARNAME "tiff")
set(PACKAGE_BUGREPORT "tiff@lists.osgeo.org")

set(CCITT_SUPPORT 1)
set(LOGLUV_SUPPORT 1)
set(LZW_SUPPORT 1)
set(MDI_SUPPORT 1)
set(NEXT_SUPPORT 1)
set(PACKBITS_SUPPORT 1)
set(THUNDER_SUPPORT 1)
set(ZIP_SUPPORT 1)

set(CXX_SUPPORT OFF)
set(JBIG_SUPPORT OFF)
set(JPEG_SUPPORT OFF)
set(JPEG_DUAL_MODE_8_12 OFF)
set(HAVE_JPEGTURBO_DUAL_MODE_8_12 OFF)
set(LERC_SUPPORT OFF)
set(LERC_STATIC OFF)
set(LIBDEFLATE_SUPPORT OFF)
set(LZMA_SUPPORT OFF)
set(OJPEG_SUPPORT OFF)
set(PIXARLOG_SUPPORT OFF)
set(WEBP_SUPPORT OFF)
set(ZSTD_SUPPORT OFF)

set(STRIPCHOP_DEFAULT 1)
set(STRIP_SIZE_DEFAULT 8192)
set(TIFF_MAX_DIR_COUNT 1048576)
set(SUBIFD_SUPPORT 1)
set(DEFAULT_EXTRASAMPLE_AS_ALPHA 1)
set(CHECK_JPEG_YCBCR_SUBSAMPLING 1)
set(CHUNKY_STRIP_READ_SUPPORT OFF)
set(DEFER_STRILE_LOAD OFF)
set(USE_WIN32_FILEIO ${WIN32})

file(COPY
    "${tiff_SOURCE_DIR}/libtiff/tiff.h"
    "${tiff_SOURCE_DIR}/libtiff/tiffio.h"
    DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/Include"
)
configure_file(
    "${tiff_SOURCE_DIR}/libtiff/tiffconf.h.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/Include/tiffconf.h"
    @ONLY
)
configure_file(
    "${tiff_SOURCE_DIR}/libtiff/tiffvers.h.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/Include/tiffvers.h"
    @ONLY
)
configure_file(
    "${tiff_SOURCE_DIR}/libtiff/tif_config.h.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/tif_config.h"
    @ONLY
)

o3de_disable_warnings()

ly_add_target(
    NAME TIFF STATIC
    NAMESPACE 3rdParty
    NO_UNITY
    FILES_CMAKE files.cmake
    OUTPUT_NAME tiff
    INCLUDE_DIRECTORIES
        PRIVATE
            "${tiff_SOURCE_DIR}/libtiff"
            "${CMAKE_CURRENT_BINARY_DIR}"
        PUBLIC "${CMAKE_CURRENT_BINARY_DIR}/Include"
    COMPILE_DEFINITIONS
        PRIVATE
            TIFF_DO_NOT_USE_NON_EXT_ALLOC_FUNCTIONS
            $<$<BOOL:${MSVC}>:_CRT_SECURE_NO_WARNINGS>
            $<$<BOOL:${MSVC}>:_CRT_NONSTDC_NO_WARNINGS>
    BUILD_DEPENDENCIES
        PRIVATE 3rdParty::zlib
)

if(UNIX OR MINGW)
    target_compile_definitions(TIFF PRIVATE _FILE_OFFSET_BITS=64)
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Linux" OR ANDROID)
    ly_target_link_libraries(TIFF PRIVATE m)
endif()

add_library(3rdParty::tiff ALIAS TIFF)
add_library(TIFF::TIFF ALIAS TIFF)
add_library(TIFF::tiff ALIAS TIFF)

set_target_properties(TIFF PROPERTIES
    C_STANDARD 99
    C_STANDARD_REQUIRED ON
    POSITION_INDEPENDENT_CODE ON
    SYSTEM TRUE
)

ly_install_files(
    FILES
        LICENSE.txt
        PackageInfo.json
    DESTINATION Code/3rdParty/tiff
)
