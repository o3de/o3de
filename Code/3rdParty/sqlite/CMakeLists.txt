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

o3de_disable_warnings()

include(CheckSymbolExists)

check_symbol_exists(log "math.h" SQLITE_MATH_IN_C_RUNTIME)
if(NOT SQLITE_MATH_IN_C_RUNTIME)
    set(CMAKE_REQUIRED_LIBRARIES m)
    check_symbol_exists(log "math.h" SQLITE_HAVE_LIBM)
    unset(CMAKE_REQUIRED_LIBRARIES)
endif()

check_symbol_exists(fdatasync "unistd.h" SQLITE_HAVE_FDATASYNC)
check_symbol_exists(gmtime_r "time.h" SQLITE_HAVE_GMTIME_R)
check_symbol_exists(localtime_r "time.h" SQLITE_HAVE_LOCALTIME_R)
check_symbol_exists(posix_fallocate "fcntl.h" SQLITE_HAVE_POSIX_FALLOCATE)
check_symbol_exists(strerror_r "string.h" SQLITE_HAVE_STRERROR_R)

find_package(Threads REQUIRED)

ly_add_target(
    NAME sqlite STATIC
    NAMESPACE 3rdParty
    NO_UNITY
    FILES_CMAKE files.cmake
    OUTPUT_NAME sqlite3
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
)

target_include_directories(sqlite SYSTEM PUBLIC "${sqlite_SOURCE_DIR}")
target_link_libraries(sqlite PUBLIC
    ${CMAKE_THREAD_LIBS_INIT}
    ${CMAKE_DL_LIBS}
    $<$<BOOL:${SQLITE_HAVE_LIBM}>:m>
)
set_target_properties(sqlite PROPERTIES C_VISIBILITY_PRESET hidden)

ly_create_alias(NAME SQLite NAMESPACE 3rdParty TARGETS 3rdParty::sqlite)

ly_install_files(
    FILES
        LICENSE.txt
        PackageInfo.json
    DESTINATION Code/3rdParty/sqlite
)
