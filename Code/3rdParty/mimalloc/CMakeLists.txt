#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#

o3de_fetch_content(mimalloc
    VERSION "3.5.2"
    LICENSE "MIT"
    URL "https://github.com/microsoft/mimalloc/archive/refs/tags/v3.5.2.tar.gz"
    URL_HASH "db5c4aaaf356edfeb1aa236b3a052fe3c67d01419db36613f783999341ed5619"
    GIT "https://github.com/microsoft/mimalloc.git"
    GIT_HASH "636510a36ab743f76a582067142f29d15b024c90"
    SOURCE_SUBDIR "no-add-subdir"
)
FetchContent_MakeAvailable(mimalloc)

file(
    COPY "${mimalloc_SOURCE_DIR}/include/"
    DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/Include"
)

o3de_disable_warnings()

ly_add_target(
    NAME mimalloc STATIC
    NAMESPACE 3rdParty
    NO_UNITY
    FILES_CMAKE files.cmake
    COMPILE_DEFINITIONS
        PRIVATE
            MI_OPT_SIMD=1
            MI_SKIP_COLLECT_ON_EXIT=1
)

target_include_directories(mimalloc SYSTEM
    PUBLIC "${CMAKE_CURRENT_BINARY_DIR}/Include"
)

target_compile_options(mimalloc PRIVATE ${O3DE_COMPILE_OPTION_ENABLE_EXCEPTIONS})

if(WIN32)
    ly_target_link_libraries(mimalloc PRIVATE advapi32)
endif()

if(NOT MSVC)
    # Match mimalloc's default to avoid a general-dynamic TLS lookup in the allocation fast path.
    target_compile_options(mimalloc PRIVATE -ftls-model=initial-exec)
endif()

ly_install_files(
    FILES
        LICENSE.txt
        PackageInfo.json
    DESTINATION Code/3rdParty/mimalloc
)
