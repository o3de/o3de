#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#

o3de_fetch_content(sqlite
    VERSION "3.53.4"
    LICENSE "blessing"
    URL "https://sqlite.org/2026/sqlite-amalgamation-3530400.zip"
    URL_HASH "1e71ddf93849c6a6ecf58b827c0692073d2dd7ee40196158068f7b29f422e87d"
)
FetchContent_MakeAvailable(sqlite)
FetchContent_GetProperties(sqlite SOURCE_DIR sqlite_source_dir)

include(CheckLibraryExists)
check_library_exists(m sin "" SQLITE_HAVE_LIBM)

include(CheckSymbolExists)
check_symbol_exists(fdatasync "unistd.h" SQLITE_HAVE_FDATASYNC)
check_symbol_exists(gmtime_r "time.h" SQLITE_HAVE_GMTIME_R)
check_symbol_exists(localtime_r "time.h" SQLITE_HAVE_LOCALTIME_R)
check_symbol_exists(posix_fallocate "fcntl.h" SQLITE_HAVE_POSIX_FALLOCATE)
check_symbol_exists(strerror_r "string.h" SQLITE_HAVE_STRERROR_R)

find_package(Threads REQUIRED)

ly_add_3rdparty_target(
    NAME sqlite STATIC
    OUTPUT_NAME sqlite3
    INCLUDE_DIRECTORIES
        PUBLIC "${sqlite_source_dir}"
    COMPILE_DEFINITIONS
        PRIVATE
            SQLITE_THREADSAFE=1
            SQLITE_ENABLE_BYTECODE_VTAB
            SQLITE_ENABLE_CARRAY
            SQLITE_ENABLE_COLUMN_METADATA
            SQLITE_ENABLE_DBPAGE_VTAB
            SQLITE_ENABLE_DBSTAT_VTAB
            SQLITE_ENABLE_FTS3
            SQLITE_ENABLE_FTS3_PARENTHESIS
            SQLITE_ENABLE_FTS5
            SQLITE_ENABLE_GEOPOLY
            SQLITE_ENABLE_MATH_FUNCTIONS
            SQLITE_ENABLE_RTREE
            SQLITE_ENABLE_STMTVTAB
            SQLITE_ENABLE_UNLOCK_NOTIFY
            $<$<BOOL:${SQLITE_HAVE_FDATASYNC}>:HAVE_FDATASYNC=1>
            $<$<BOOL:${SQLITE_HAVE_GMTIME_R}>:HAVE_GMTIME_R=1>
            $<$<BOOL:${SQLITE_HAVE_LOCALTIME_R}>:HAVE_LOCALTIME_R=1>
            $<$<BOOL:${SQLITE_HAVE_POSIX_FALLOCATE}>:HAVE_POSIX_FALLOCATE=1>
            $<$<BOOL:${SQLITE_HAVE_STRERROR_R}>:HAVE_STRERROR_R=1>
    BUILD_DEPENDENCIES
        PUBLIC
            ${CMAKE_THREAD_LIBS_INIT}
            ${CMAKE_DL_LIBS}
            $<$<BOOL:${SQLITE_HAVE_LIBM}>:m>
    TARGET_PROPERTIES
        C_VISIBILITY_PRESET hidden
)

ly_create_alias(NAME SQLite NAMESPACE 3rdParty TARGETS 3rdParty::sqlite)
