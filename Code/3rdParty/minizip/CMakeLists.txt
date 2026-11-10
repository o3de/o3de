#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#

o3de_fetch_content(minizip
    VERSION "4.2.2"
    LICENSE "Zlib"
    URL "https://github.com/zlib-ng/minizip-ng/archive/7b2387161c542fa9f427352dcdef76097d0d692b.tar.gz"
    URL_HASH "467a189e998c7754ada577758d29324d5b3c866a7fb5d43700b8c551d72ca998"
    GIT "https://github.com/zlib-ng/minizip-ng.git"
    GIT_HASH "7b2387161c542fa9f427352dcdef76097d0d692b"
    NO_ADD_SUBDIR
)
FetchContent_MakeAvailable(minizip)

o3de_disable_warnings()

include(CheckIncludeFile)
include(CheckSymbolExists)

foreach(header dirent.h sys/dirent.h inttypes.h stdint.h)
    string(MAKE_C_IDENTIFIER "${header}" header_id)
    string(TOUPPER "${header_id}" header_id)
    check_include_file("${header}" MZ_HAVE_${header_id})
    set(HAVE_${header_id} "${MZ_HAVE_${header_id}}")
endforeach()

set(HAVE_PDIR "${HAVE_DIRENT_H}")

foreach(symbol fseeko symlink readlink)
    string(TOUPPER "${symbol}" symbol_id)
    check_symbol_exists("${symbol}" "stdio.h;unistd.h" MZ_HAVE_${symbol_id})
    set(HAVE_${symbol_id} "${MZ_HAVE_${symbol_id}}")
endforeach()

file(COPY
    "${minizip_SOURCE_DIR}/"
    DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/Include/minizip"
    FILES_MATCHING
        PATTERN "*.h"
        PATTERN "test" EXCLUDE
        PATTERN ".github" EXCLUDE
        PATTERN "doc" EXCLUDE
)
file(COPY
    "${minizip_SOURCE_DIR}/compat/ioapi.h"
    "${minizip_SOURCE_DIR}/compat/unzip.h"
    "${minizip_SOURCE_DIR}/compat/zip.h"
    DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/Include/minizip"
)
configure_file(
    "${minizip_SOURCE_DIR}/mz_config.h.cmakein"
    "${CMAKE_CURRENT_BINARY_DIR}/Include/minizip/mz_config.h"
    @ONLY
)

ly_add_target(
    NAME minizip STATIC
    NAMESPACE 3rdParty
    NO_UNITY
    FILES_CMAKE files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/Include/minizip"
        PUBLIC "${CMAKE_CURRENT_BINARY_DIR}/Include"
    COMPILE_DEFINITIONS
        PRIVATE
            HAVE_ZLIB
            ZLIB_COMPAT
            MZ_ZIP_NO_CRYPTO
            MZ_ZIP_NO_ENCRYPTION
            _CRT_NONSTDC_NO_DEPRECATE
            _CRT_SECURE_NO_DEPRECATE
            $<$<NOT:$<BOOL:${WIN32}>>:_FILE_OFFSET_BITS=64>
    BUILD_DEPENDENCIES
        PUBLIC 3rdParty::zlib
)

set_target_properties(minizip PROPERTIES
    POSITION_INDEPENDENT_CODE ON
    SYSTEM TRUE
    C_STANDARD 99
    C_STANDARD_REQUIRED ON
)

ly_install_files(
    FILES
        LICENSE.txt
        PackageInfo.json
    DESTINATION Code/3rdParty/minizip
)
